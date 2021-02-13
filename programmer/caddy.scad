$fs = 0.01; // min angle for curved shapes
$fa = 2; // min segment/fragment size

RAD = 180 / PI;
DEG = 1;

epsilon = 0.001; // fudge to fix floating point rounding issues?

module caddy() {
    module arc(r1, r2, theta, h, taper = 0, center=false) {
      rotation = (center ? -theta/2 : 0);
      rotate(rotation, [0,0,1]) rotate_extrude(angle=theta)
        translate([r1,0])
          polygon([[0,0], [r2-r1,0], [r2-r1-taper, h], [0,h]]);
          //square(abs(r2-r1));
    };
    
    r0 = 20; // caddy circle
    r1 = 24; // board diameter
    r2 = 27; // outer arms to cradle board
    
    arms_height = 6;
    base_height = 2;
    rest_height = 5.2 - base_height; // pogopin recommended working height is 5.57mm, min=4.87

    screwhole_radius = 1.5;
    
    spoke_arclen = PI/3*RAD;
    arm_arclen = PI/8*RAD;
    
    spoke_centers = [-PI/2*RAD,
                     -5*PI/4*RAD,
                     -7*PI/4*RAD,
                    ];
    
    difference() {
        union() {
            for (i = [0:len(spoke_centers)-1]) {
                rotate(spoke_centers[i], [0,0,1]) {
                    width = 2*r2 * sin(arm_arclen/2);
                    sides = [spoke_arclen/2 + arm_arclen/2, -spoke_arclen/2 - arm_arclen/2];
                    for (s = [0:len(sides)-1]) rotate(sides[s], [0,0,1]) {
                        // base
                        arc(r0, r2, arm_arclen, base_height, taper=0, center=true);
                        translate([0,0,base_height]) {
                            // arm
                            arc(r1, r2, arm_arclen, arms_height, taper=2, center=true);
                            // rest
                            arc(r0-2, r0, arm_arclen, rest_height, taper=1, center=true);
                        }
                    }
                }
            }
            difference() {
                cylinder(r=r0, h=base_height);
                for (i = [0:len(spoke_centers)-1]) {
                    rotate(spoke_centers[i], [0,0,1]) translate([14, 0, -epsilon]) scale([1,1,1+epsilon]) cylinder(r=screwhole_radius, h=base_height);
                }
            }
        }
        translate([0, 0, -epsilon]) scale([1,1,1+epsilon]) cylinder(r=8, h=base_height);
    } 
}

caddy();