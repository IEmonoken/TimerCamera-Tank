n = 11;
ra = 11.0 * n / 11;
rb = 9.5 * n / 11;
rc = rb - 1.8;

gearPoints = [
  [-1, -2,   -1.4],
  [ 2, -0.4, -1  ],
  [ 2,  0.4, -1  ],
  [-1,  2,   -1.4],
  [-1, -2,    1.4],
  [ 2, -0.4,  1  ],
  [ 2,  0.4,  1  ],
  [-1,  2,    1.4]];
  
gearFaces = [
  [0,1,2,3],
  [4,5,1,0],
  [7,6,5,4],
  [5,6,2,1],
  [6,7,3,2],
  [7,4,0,3]];

difference() {
    union() {
        difference() {
            union() {
                translate([0, 0, 5.5]) cylinder(h = 6, r = ra, center = true, $fn = 100);
                translate([0, 0, 5.5]) cylinder(h = 11, r = rb, center = true, $fn = 100);
                for (i = [0 : n - 1]) {
                    rotate(i * 360 / n, [0, 0, 1])
                        translate([ra, 0, 5.5]) polyhedron(gearPoints, gearFaces);
                }
            }
            translate([0, 0, 10]) cylinder(h = 10, r = rc, center = true, $fn = 100);
        }
        translate([0, 0, 9.5]) cylinder(h = 19,r = 5, center = true, $fn = 100);
    }
    translate([0, 0, 9.5]) cylinder(h = 20,r = 1.4, center = true, $fn = 50);
    rotate(90, [0, 1, 0])
        translate([-15, 0, 0]) cylinder(h = 20,r = 0.7, center = true, $fn = 20);
}

