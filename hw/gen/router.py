"""
router.py - a small two-layer grid maze router (A*, 45-degree moves).

Good enough for a board with ~15 signal nets and lots of room: every
obstacle is inflated by (clearance + half track width), so any grid path
that avoids blocked cells is DRC-clean by construction.  GND is not routed;
it comes from copper pours on both layers.
"""
import heapq
import math

import numpy as np

SQ2 = math.sqrt(2.0)


class Router:
    def __init__(self, bbox, res, clearance, edge_clear, via_d, margin=None,
                 layer_cost=(1.0, 1.6), via_cost=14.0):
        x0, y0, x1, y1 = bbox
        self.res = res
        self.x0, self.y0 = x0, y0
        self.nx = int(math.ceil((x1 - x0) / res)) + 1
        self.ny = int(math.ceil((y1 - y0) / res)) + 1
        self.N = self.nx * self.ny
        xs = x0 + np.arange(self.nx) * res
        ys = y0 + np.arange(self.ny) * res
        self.X, self.Y = np.meshgrid(xs, ys)
        self.clear = clearance
        self.edge_clear = edge_clear
        self.via_r = via_d / 2
        # diagonal steps cut corners by up to res/sqrt(2)/2 - pad for it
        self.margin = margin if margin is not None else res * 0.45
        self.layer_cost = layer_cost
        self.via_cost = via_cost
        self.shapes = []
        self.edge_dist = None
        self.paths = {}        # net -> list of (layer, [(x,y),...])
        self.vias = {}         # net -> [(x,y)]
        self.tree = {}         # net -> set(flat idx)

    # ---------------------------------------------------------------- geometry
    def cell(self, x, y):
        return (int(round((x - self.x0) / self.res)), int(round((y - self.y0) / self.res)))

    def xy(self, i, j):
        return (self.x0 + i * self.res, self.y0 + j * self.res)

    def set_edge_distance(self, fn):
        """fn(X, Y) -> signed distance to nearest board edge (>0 inside copper area)."""
        self.edge_dist = fn(self.X, self.Y)

    def add_circle(self, net, layers, x, y, r):
        self.shapes.append(("c", net, tuple(layers), (x, y, r)))

    def add_rect(self, net, layers, x, y, hw, hh):
        self.shapes.append(("r", net, tuple(layers), (x, y, hw, hh)))

    def add_capsule(self, net, layers, x1, y1, x2, y2, r):
        self.shapes.append(("s", net, tuple(layers), (x1, y1, x2, y2, r)))

    def _sdf_box(self, kind, p, inflate):
        """Return (j0, j1, i0, i1, mask) of cells within `inflate` of the shape."""
        if kind == "c":
            x, y, r = p
            ext = r + inflate
            bx0, by0, bx1, by1 = x - ext, y - ext, x + ext, y + ext
        elif kind == "r":
            x, y, hw, hh = p
            bx0, by0, bx1, by1 = x - hw - inflate, y - hh - inflate, x + hw + inflate, y + hh + inflate
        else:
            x1, y1, x2, y2, r = p
            ext = r + inflate
            bx0, by0 = min(x1, x2) - ext, min(y1, y2) - ext
            bx1, by1 = max(x1, x2) + ext, max(y1, y2) + ext
        i0 = max(0, int(math.floor((bx0 - self.x0) / self.res)))
        i1 = min(self.nx, int(math.ceil((bx1 - self.x0) / self.res)) + 1)
        j0 = max(0, int(math.floor((by0 - self.y0) / self.res)))
        j1 = min(self.ny, int(math.ceil((by1 - self.y0) / self.res)) + 1)
        if i0 >= i1 or j0 >= j1:
            return None
        X = self.X[j0:j1, i0:i1]
        Y = self.Y[j0:j1, i0:i1]
        if kind == "c":
            d = np.hypot(X - x, Y - y) - r
        elif kind == "r":
            dx = np.maximum(np.abs(X - x) - hw, 0)
            dy = np.maximum(np.abs(Y - y) - hh, 0)
            d = np.hypot(dx, dy)
        else:
            vx, vy = x2 - x1, y2 - y1
            L2 = vx * vx + vy * vy
            if L2 < 1e-12:
                t = np.zeros_like(X)
            else:
                t = np.clip(((X - x1) * vx + (Y - y1) * vy) / L2, 0, 1)
            d = np.hypot(X - (x1 + t * vx), Y - (y1 + t * vy)) - r
        return j0, j1, i0, i1, d <= inflate

    def blocked(self, net, w):
        inf_t = self.clear + w / 2 + self.margin
        inf_v = self.clear + self.via_r + self.margin
        bt = [self.edge_dist < self.edge_clear + w / 2 + self.margin for _ in range(2)]
        bv = self.edge_dist < self.edge_clear + self.via_r + self.margin
        for kind, snet, layers, p in self.shapes:
            if snet is not None and snet == net:
                continue
            r = self._sdf_box(kind, p, inf_t)
            if r:
                j0, j1, i0, i1, m = r
                for L in layers:
                    bt[L][j0:j1, i0:i1] |= m
            r = self._sdf_box(kind, p, inf_v)
            if r:
                j0, j1, i0, i1, m = r
                bv[j0:j1, i0:i1] |= m
        return bt, bv

    # ------------------------------------------------------------------ search
    def pad_cells(self, x, y, r, layers):
        """Grid cells comfortably inside a pad of radius r, on given layers."""
        out = []
        rr = max(r - 0.1, self.res * 0.5)
        n = int(math.ceil(rr / self.res))
        ci, cj = self.cell(x, y)
        for dj in range(-n, n + 1):
            for di in range(-n, n + 1):
                i, j = ci + di, cj + dj
                px, py = self.xy(i, j)
                if math.hypot(px - x, py - y) <= rr:
                    for L in layers:
                        out.append(L * self.N + j * self.nx + i)
        if not out:
            out = [L * self.N + cj * self.nx + ci for L in layers]
        return out

    def astar(self, net, w, starts, targets, bt, bv):
        nx, N = self.nx, self.N
        free0 = bytearray((~bt[0]).astype(np.uint8).ravel().tobytes())
        free1 = bytearray((~bt[1]).astype(np.uint8).ravel().tobytes())
        freev = bytearray((~bv).astype(np.uint8).ravel().tobytes())
        free = (free0, free1)
        tgt = set(targets)
        for t in tgt:          # own pads / own tree always enterable
            free[t // N][t % N] = 1
        starts = list(starts)
        for s in starts:
            free[s // N][s % N] = 1
        # heuristic anchors
        tl = list(tgt)
        if len(tl) > 80:
            step = len(tl) // 80 + 1
            tl = tl[::step]
        anchors = [((t % N) % nx, (t % N) // nx) for t in tl]
        lc = self.layer_cost
        hmin = min(lc)

        def h(i, j):
            best = 1e18
            for ai, aj in anchors:
                dx = abs(ai - i)
                dy = abs(aj - j)
                v = (dx + dy) + (SQ2 - 2) * (dx if dx < dy else dy)
                if v < best:
                    best = v
            return best * hmin * 0.98

        g = {}
        came = {}
        heap = []
        for s in starts:
            g[s] = 0.0
            came[s] = -1
            k = s % N
            heapq.heappush(heap, (h(k % nx, k // nx), 0.0, s))
        moves = ((1, 0, 1.0), (-1, 0, 1.0), (0, 1, 1.0), (0, -1, 1.0),
                 (1, 1, SQ2), (1, -1, SQ2), (-1, 1, SQ2), (-1, -1, SQ2))
        ny = self.ny
        closed = set()
        found = None
        while heap:
            f, gc, s = heapq.heappop(heap)
            if s in closed:
                continue
            closed.add(s)
            if s in tgt:
                found = s
                break
            L = s // N
            k = s - L * N
            i = k % nx
            j = k // nx
            fl = free[L]
            c = lc[L]
            for di, dj, dc in moves:
                ii = i + di
                jj = j + dj
                if ii < 0 or jj < 0 or ii >= nx or jj >= ny:
                    continue
                kk = jj * nx + ii
                if not fl[kk]:
                    continue
                if di and dj and not (fl[j * nx + ii] or fl[jj * nx + i]):
                    continue
                ns = L * N + kk
                ng = gc + dc * c
                if ng < g.get(ns, 1e18):
                    g[ns] = ng
                    came[ns] = s
                    heapq.heappush(heap, (ng + h(ii, jj), ng, ns))
            # via
            if freev[k]:
                ol = 1 - L
                ns = ol * N + k
                if free[ol][k]:
                    ng = gc + self.via_cost
                    if ng < g.get(ns, 1e18):
                        g[ns] = ng
                        came[ns] = s
                        heapq.heappush(heap, (ng + h(i, j), ng, ns))
        if found is None:
            return None
        path = []
        s = found
        while s != -1:
            path.append(s)
            s = came[s]
        path.reverse()
        return path

    # --------------------------------------------------------------- commit
    def commit(self, net, w, path):
        N, nx = self.N, self.nx
        runs = []
        vias = []
        cur_L = None
        cur = []
        for s in path:
            L = s // N
            k = s % N
            p = (k % nx, k // nx)
            if cur_L is None:
                cur_L = L
                cur = [p]
            elif L != cur_L:
                vias.append(p)
                if len(cur) > 1:
                    runs.append((cur_L, cur))
                cur_L = L
                cur = [p]
            else:
                cur.append(p)
        if len(cur) > 1:
            runs.append((cur_L, cur))
        out = []
        for L, pts in runs:
            simp = [pts[0]]
            for a, b, c in zip(pts, pts[1:], pts[2:]):
                if (b[0] - a[0], b[1] - a[1]) != (c[0] - b[0], c[1] - b[1]):
                    simp.append(b)
            simp.append(pts[-1])
            xy = [self.xy(*p) for p in simp]
            out.append((L, xy))
            for (x1, y1), (x2, y2) in zip(xy, xy[1:]):
                self.add_capsule(net, (L,), x1, y1, x2, y2, w / 2)
        vxy = [self.xy(*p) for p in vias]
        for x, y in vxy:
            self.add_circle(net, (0, 1), x, y, self.via_r)
        self.paths.setdefault(net, []).extend(out)
        self.vias.setdefault(net, []).extend(vxy)
        t = self.tree.setdefault(net, set())
        t.update(path)
        for x, y in vxy:
            i, j = self.cell(x, y)
            t.add(j * nx + i)
            t.add(N + j * nx + i)
        return out, vxy

    def route_net(self, net, w, pads):
        """pads: list of (x, y, r, layers).  Builds a tree pad by pad."""
        if len(pads) < 2:
            return True
        bt, bv = self.blocked(net, w)
        # tree starts at the pad nearest the centroid of all pads
        cx = sum(p[0] for p in pads) / len(pads)
        cy = sum(p[1] for p in pads) / len(pads)
        order = sorted(pads, key=lambda p: math.hypot(p[0] - cx, p[1] - cy))
        first = order[0]
        tree = set(self.pad_cells(first[0], first[1], first[2], first[3]))
        self.tree.setdefault(net, set()).update(tree)
        todo = order[1:]
        ok = True
        while todo:
            # nearest remaining pad to the current tree (cheap proxy: to first pad)
            todo.sort(key=lambda p: min(math.hypot(p[0] - q[0], p[1] - q[1]) for q in pads
                                        if q not in todo))
            p = todo.pop(0)
            starts = self.pad_cells(p[0], p[1], p[2], p[3])
            path = self.astar(net, w, starts, self.tree[net], bt, bv)
            if path is None:
                print(f"    !! could not route {net} to pad at ({p[0]:.2f},{p[1]:.2f})")
                ok = False
                continue
            self.commit(net, w, path)
            self.tree[net].update(starts)
        return ok
