$fs = 0.01; // min angle for curved shapes
$fa = 2; // min segment/fragment size

RAD = 180 / PI;
DEG = 1;

epsilon = 0.001; // fudge to fix floating point rounding issues?
tolerance = 0.2;  // 3D printing tolerance to fit around objects

module diffscale() {
     for (i = [0 : $children-1]) {
         translate([-epsilon/2,-epsilon/2,-epsilon/2]) scale([1+epsilon,1+epsilon,1+epsilon]) children(i);
     }
}

module rounded_rect(size, radius, center=false, epsilon=0.001) {
    centertranslation = center ? [-size.x/2, -size.y/2, -size.z/2] : [0,0,0];
    module fillet(r, h) {
        translate([r/2, r/2, 0]) difference() {
            cube([r + epsilon, r + epsilon, h], center = true);
            translate([r/2, r/2, 0])
                cylinder(r = r, h = h + 1, center = true);
        }
    }
    translate(centertranslation) difference() {
        cube(size);
        translate([0,0,size.z/2]) fillet(radius,size.z+0.001);
        translate([size.x,0,size.z/2]) rotate(PI/2*RAD, [0,0,1]) fillet(radius, size.z+epsilon);
        translate([0,size.y,size.z/2]) rotate(-PI/2*RAD, [0,0,1]) fillet(radius, size.z+epsilon);
        translate([size.x,size.y,size.z/2]) rotate(PI*RAD, [0,0,1]) fillet(radius, size.z+epsilon);
    }
}

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
    r2 = 27+tolerance; // outer arms to cradle board
    
    arms_height = 6;
    base_height = 3;
    rest_height = 5.2 - base_height; // pogopin recommended working height is 5.57mm, min=4.87

    screwhole_radius = 1.25;
    
    spoke_arclen = PI/3*RAD;
    arm_arclen = PI/8*RAD;
    
    spoke_centers = [-PI/2*RAD,
                     -5*PI/4*RAD,
                     -7*PI/4*RAD,
                    ];
    
    drill_centers = [-PI/2*RAD,
                     -9*PI/8*RAD,
                     -15*PI/8*RAD,
                    ];
    
    bar_pin_cutout_size = [25.3 /*meas*/ + 2*1.75 /*asym*/ + 1.4/*tol*/,
                        5.1 /*meas*/ + 0.6 /*tol*/,
                        base_height];
    bar_pin_cutout_position = [0, 13, 0];
    
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
                            arc(r0-2-epsilon, r0, arm_arclen, rest_height, taper=1, center=true);
                        }
                    }
                }
            }
            cylinder(r=r0, h=base_height);
            
            translate(bar_pin_cutout_position + [0, 0, bar_pin_cutout_size.z/2]) {
                scale([1.2, 1.9, 1]) rounded_rect(bar_pin_cutout_size, 1, center=true);
            }
        }
        union() {
            translate([0, 0, -epsilon]) scale([1,1,1+epsilon]) cylinder(r=8, h=base_height);
            for (i = [0:len(drill_centers)-1]) {
                rotate(drill_centers[i], [0,0,1]) translate([14, 0, -epsilon]) scale([1,1,1+epsilon]) cylinder(r=screwhole_radius, h=base_height);
            }
            
            // space for bar pin
            diffscale() translate(bar_pin_cutout_position - [bar_pin_cutout_size.x/2, bar_pin_cutout_size.y/2]) {
                cube(bar_pin_cutout_size);
            }
        }
    } 
}

caddy();