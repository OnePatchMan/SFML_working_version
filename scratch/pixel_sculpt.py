import math
from PIL import Image, ImageDraw

class PixelLayer:
    """Pixel-art sculpting layer with organic tapered curves, volume shading, and crisp 1px outlining."""
    def __init__(self, w, h):
        self.w = w
        self.h = h
        self.img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        self.px = self.img.load()

    def set_px(self, x, y, color):
        ix, iy = int(round(x)), int(round(y))
        if 0 <= ix < self.w and 0 <= iy < self.h:
            self.px[ix, iy] = color

    def get_px(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.px[x, y]
        return (0, 0, 0, 0)

    def disc(self, cx, cy, r, color):
        if r <= 0.4:
            self.set_px(cx, cy, color)
            return
        x0 = max(0, int(math.floor(cx - r)))
        x1 = min(self.w - 1, int(math.ceil(cx + r)))
        y0 = max(0, int(math.floor(cy - r)))
        y1 = min(self.h - 1, int(math.ceil(cy + r)))
        r2 = r * r + 0.25
        for y in range(y0, y1 + 1):
            dy = y - cy
            for x in range(x0, x1 + 1):
                dx = x - cx
                if dx * dx + dy * dy <= r2:
                    self.px[x, y] = color

    def tapered_limb(self, nodes, color):
        """
        Draws a smooth organic limb/spine from a list of (x, y, radius) tuples.
        Interpolates densely between nodes so muscles taper and bulge naturally.
        """
        if len(nodes) < 2:
            if len(nodes) == 1:
                self.disc(nodes[0][0], nodes[0][1], nodes[0][2], color)
            return
        for i in range(len(nodes) - 1):
            n0 = nodes[i]
            n1 = nodes[i + 1]
            x0, y0, r0 = n0[0], n0[1], (n0[2] if len(n0) > 2 else 1.8)
            x1, y1, r1 = n1[0], n1[1], (n1[2] if len(n1) > 2 else 1.8)
            dist = math.hypot(x1 - x0, y1 - y0)
            steps = max(2, int(dist * 2.5))
            for s in range(steps + 1):
                t = s / steps
                x = x0 + (x1 - x0) * t
                y = y0 + (y1 - y0) * t
                r = r0 + (r1 - r0) * t
                self.disc(x, y, r, color)

    def bezier_limb(self, p0, p1, p2, r0, r1, r2, color):
        """Draws a smooth quadratic Bezier tapered limb/spine."""
        dist = math.hypot(p1[0] - p0[0], p1[1] - p0[1]) + math.hypot(p2[0] - p1[0], p2[1] - p1[1])
        steps = max(4, int(dist * 2.5))
        for s in range(steps + 1):
            t = s / steps
            u = 1.0 - t
            x = u * u * p0[0] + 2 * u * t * p1[0] + t * t * p2[0]
            y = u * u * p0[1] + 2 * u * t * p1[1] + t * t * p2[1]
            r = u * u * r0 + 2 * u * t * r1 + t * t * r2
            self.disc(x, y, r, color)

    def polygon(self, pts, color):
        d = ImageDraw.Draw(self.img)
        d.polygon([(int(round(x)), int(round(y))) for x, y in pts], fill=color)

    def line(self, pts, color, width=1):
        d = ImageDraw.Draw(self.img)
        d.line([(int(round(x)), int(round(y))) for x, y in pts], fill=color, width=width)

    def auto_shade(self, base_color, light_color, hi_color, shadow_color, deep_color):
        """
        Applies directional pixel-art shading (top-left light, bottom-right shadow)
        to all pixels currently matching `base_color`.
        """
        orig = self.img.copy().load()
        for y in range(self.h):
            for x in range(self.w):
                if orig[x, y] != base_color:
                    continue
                # Check distance to empty space in top-left vs bottom-right
                up = (y == 0 or orig[x, y - 1][3] == 0)
                left = (x == 0 or orig[x - 1, y][3] == 0)
                up2 = (y <= 1 or orig[x, y - 2][3] == 0)
                left2 = (x <= 1 or orig[x - 2, y][3] == 0)
                down = (y == self.h - 1 or orig[x, y + 1][3] == 0)
                right = (x == self.w - 1 or orig[x + 1, y][3] == 0)
                down2 = (y >= self.h - 2 or orig[x, y + 2][3] == 0)
                right2 = (x >= self.w - 2 or orig[x + 2, y][3] == 0)

                if up and left:
                    self.px[x, y] = hi_color
                elif up or left:
                    self.px[x, y] = light_color
                elif up2 and left2:
                    self.px[x, y] = light_color
                elif down and right:
                    self.px[x, y] = deep_color
                elif down or right:
                    self.px[x, y] = shadow_color
                elif down2 and right2:
                    self.px[x, y] = shadow_color

    def add_outline(self, outline_color, ground_y=None):
        """
        Adds a crisp 1px 4-connected outline around opaque pixels.
        If ground_y is set, prevents outline from bleeding below ground_y + 1.
        """
        orig = self.img.copy().load()
        max_y = self.h - 1 if ground_y is None else min(self.h - 1, ground_y)
        for y in range(max_y + 1):
            for x in range(self.w):
                if orig[x, y][3] > 0:
                    continue
                has_neighbor = False
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    if 0 <= nx < self.w and 0 <= ny < self.h and orig[nx, ny][3] > 0:
                        has_neighbor = True
                        break
                if has_neighbor:
                    self.px[x, y] = outline_color

def composite_layers(w, h, layers):
    out = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    for layer in layers:
        out.alpha_composite(layer.img)
    return out
