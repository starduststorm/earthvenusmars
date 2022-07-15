// earthvenusmars case design

$fs = 0.05; // min angle for curved shapes
$fa = 3; // min segment/fragment size

RAD = 180 / PI;
DEG = 1;

epsilon = 0.001; // fudge to fix floating point rounding issues?
tolerance = 0.2;  // 3D printing tolerance to fit around objects

colors = ["#F15570", "white", "#2A9FFA"];

module diffscale() {
     for (i = [0 : $children-1]) {
         translate([-epsilon/2,-epsilon/2,-epsilon/2]) scale([1+epsilon,1+epsilon,1+epsilon]) children(i);
     }
}

function easeInOutQubic(t, b, c, d) = 
	let (t = t / (d/2))
	(t < 1
        ? c/2*t*t*t + b
        : c/2*((t-2)*(t-2)*(t-2) + 2) + b); // unused

function easeOutCubic(t, b, c, d) =
	let (t = t/d, t2 = t-1)
        c*(t2*t2*t2 + 1) + b;

function circle_pt(theta, radius) = [radius*cos(theta), radius*sin(theta), 0];
function angle (a,b)=atan2(norm(cross(a,b)),a*b);

edge_cut_radius = 24 + tolerance;
spoke_arclen = PI/3*RAD;

outeroffset = 1.8;  // max distance outside of board edge
inneroffset = -1.2; // max distance overlapping from board edge

encased_spoke_width = abs(circle_pt(spoke_arclen/2, edge_cut_radius)[1] - circle_pt(-spoke_arclen/2, edge_cut_radius)[1]);
max_outer_spoke_width = encased_spoke_width + 2 * outeroffset; 

spoke_length = 20+tolerance;
spokes = [PI/2*RAD, 5*PI/4*RAD, 7*PI/4*RAD];

module arc(arcstart, arclen, height, r1, r2=0) {
    rotate(arcstart, [0,0,1]) rotate_extrude(angle=arclen) polygon([[r1,0],[r2,0],[r2,height],[r1,height]]);
}

module rounded_rect(size, radius, epsilon=0.001) {
    module fillet(r, h) {
        translate([r/2, r/2, 0]) difference() {
            cube([r + epsilon, r + epsilon, h], center = true);
            translate([r/2, r/2, 0])
                cylinder(r = r, h = h + 1, center = true);
        }
    }
    difference() {
        cube(size);
        translate([0,0,size.z/2]) fillet(radius,size.z+0.001);
        translate([size.x,0,size.z/2]) rotate(PI/2*RAD, [0,0,1]) fillet(radius, size.z+epsilon);
        translate([0,size.y,size.z/2]) rotate(-PI/2*RAD, [0,0,1]) fillet(radius, size.z+epsilon);
        translate([size.x,size.y,size.z/2]) rotate(PI*RAD, [0,0,1]) fillet(radius, size.z+epsilon);
    }
}

module theshape(outeroffset, thickness, inneroffset, zoffset, base_cutouts=false) {
    difference() {
        translate([0,0,zoffset])
        union() {
            //translate([0,0,5]) cylinder(r = 20, h = thickness);
            cylinder(r = edge_cut_radius + outeroffset, h = thickness);
            for (s = [0:2]) {
                color(colors[s]) rotate(-spokes[s], [0,0,1]) translate(circle_pt(spoke_arclen/2, edge_cut_radius)) rotate(-90, [0,0,1])
                    translate([-outeroffset,0, 0])
                    union() {
                        let (outer_spoke_width = encased_spoke_width + 2 * outeroffset) {
                            cube([outer_spoke_width, spoke_length, thickness]);
                            translate([outer_spoke_width/2, spoke_length, 0])    
                                cylinder(r = outer_spoke_width/2, h = thickness);
                        }
                    }
            } 
        };
        
        if (!base_cutouts) translate([0,0,zoffset-epsilon]) union() {
            cylinder(r = edge_cut_radius + inneroffset, h = thickness+2*epsilon);
            for (s = [0:2]) {
                color(colors[s]) rotate(-spokes[s], [0,0,1]) translate(circle_pt(spoke_arclen/2, edge_cut_radius)) rotate(-90, [0,0,1])
                    union() {
                        translate([-inneroffset, inneroffset, 0])
                        cube([encased_spoke_width + 2 * inneroffset, spoke_length - inneroffset, thickness+2*epsilon]);
                        translate([encased_spoke_width/2, spoke_length, 0]) let (radius = encased_spoke_width/2 + inneroffset) {
                            // echo("$vpt = ", $vpt, ", bottom: ", $vpt.y + radius);
                            cylinder(r = radius, h = thickness+2*epsilon);
                        }
                    }
            }
        };
    }
};

boardthickness = 1.6 + tolerance;
bevel_max_thickness = 1.5;
basethickness = 1.8;
extra_edge_height = 4.4; // to line up with bolted-on diffuser
slicethickness = 0.15; // the likely minimum resolution of the printer is 0.1mm

part_placement_offset = 34;

diffuser_top_extra_thickness = 0.8;
diffuser_top_line_width = 2.0;

led_thickness = 0.8 + tolerance; // datasheet says 0.9mm but I measure 0.8??
diffuser_outer_line_width = 5;
diffuser_inner_line_width = 2.4;
led_radius = 20;

cross_start_radius = [37.5, 30.5, 0];
cross_outer_length = 18.6;
cross_inner_length = cross_outer_length - diffuser_outer_line_width + diffuser_inner_line_width;

arrow_start_radius = [0, 49, 49];
arrow_outer_side_width = 15.1;
arrow_inner_side_width = arrow_outer_side_width - diffuser_outer_line_width + diffuser_inner_line_width;

arrow_outer_point_adjustment = 1.9;
arrow_inner_point_adjustment = 1.08;

cross_inner_adjustment = -3.2;
cross_outer_adjustment = cross_inner_adjustment - (diffuser_inner_line_width - diffuser_outer_line_width) / 2;

module led_outline(outer_radius, thickness, line_width, arrow_side_width, cross_length, arrow_point_adjustment, cross_length_adjustment) {
    difference() {
        union() {
            cylinder(h = thickness, r=outer_radius);
            for (s = [0:2]) {
                dick_shortening_adjustment = arrow_start_radius[s] != 0 ? -5 : -2 + cross_length_adjustment;
                rotate(-spokes[s], [0,0,1]) translate([0,-line_width/2,0])
                    translate([led_radius, 0,0]) cube([spoke_length + max_outer_spoke_width/2 + dick_shortening_adjustment, line_width, thickness]);
                
                if (cross_start_radius[s] != 0) {
                    rotate(-spokes[s], [0,0,1]) translate([cross_start_radius[s] - line_width/2, -cross_length/2,0])
                        cube([line_width, cross_length, thickness]);
                }
                if (arrow_start_radius[s] != 0) {
                    rotate(-spokes[s], [0,0,1]) translate([arrow_start_radius[s], 0, 0]) {
                        rotate(45*DEG,[0,0,1]) translate([-line_width/2, -arrow_point_adjustment, 0])
                            cube([line_width, arrow_side_width + arrow_point_adjustment, thickness]);
                        rotate(135*DEG,[0,0,1]) translate([-line_width/2, -arrow_point_adjustment, 0])
                            cube([line_width, arrow_side_width + arrow_point_adjustment, thickness]);
                    }
                }
            }
        }
        diffscale() cylinder(h = thickness+epsilon, r=led_radius - line_width/2);
    }
}

module top_case(flip) {
    translate([part_placement_offset, 0,0]) rotate(PI*RAD, [0,0,1]) {
        difference() {
            union() {
                // diffuser
                led_outline(led_radius + diffuser_outer_line_width/2 /*edge_cut_radius*/, bevel_max_thickness, diffuser_outer_line_width, arrow_outer_side_width, cross_outer_length, arrow_outer_point_adjustment, cross_outer_adjustment);
                translate([0,0,bevel_max_thickness-epsilon]) 
                    led_outline(led_radius + diffuser_top_line_width/2, diffuser_top_extra_thickness, diffuser_top_line_width, arrow_inner_side_width, cross_inner_length, arrow_inner_point_adjustment, cross_inner_adjustment);
            }
            union() {
                // diffuser cutout
                diffscale() led_outline(led_radius + diffuser_inner_line_width/2, led_thickness, diffuser_inner_line_width, arrow_inner_side_width, cross_inner_length, arrow_inner_point_adjustment, cross_inner_adjustment);
            }
        }
    }
};

module trans_symbol(radius, thickness) {
    linewidth=0.2*radius;
    linelength=2.4*radius;
    corner=0.07*radius;
    module arrowshape() {
        translate([linelength, 0,0]) {
            rotate(3*PI/4*RAD, [0,0,1]) translate([-linewidth/2,-linewidth/2,0]) rounded_rect([linelength/3,linewidth,thickness], corner);
            rotate(5*PI/4*RAD, [0,0,1]) translate([-linewidth/2,-linewidth/2,0]) rounded_rect([linelength/3,linewidth,thickness], corner);
        }
    }
    module crossshape(offset=0) {
        translate([10*linelength/16+offset, -0.4*linelength/2, 0]) rotate(PI/2*RAD,[0,0,1]) rounded_rect([0.4*linelength,linewidth,thickness], corner);
    }
    difference() {
        union() {
            translate([0,0,thickness/2]) cylinder(h=thickness, r=radius, center=true);
            rotate(-1*PI/2*RAD, [0,0,1]) { translate([0,-linewidth/2,0]) rounded_rect([linelength, linewidth, thickness], corner); crossshape(offset=0.15*radius); };
            rotate(-5*PI/4*RAD, [0,0,1]) { translate([0,-linewidth/2,0]) rounded_rect([linelength, linewidth, thickness], corner); arrowshape(); };
            rotate(-7*PI/4*RAD, [0,0,1]) { translate([0,-linewidth/2,0]) rounded_rect([linelength, linewidth, thickness], corner); arrowshape(); crossshape(); };
        }
        translate([0,0,thickness/2]) cylinder(h=thickness, r=radius-linewidth, center=true);
    }
}

thumbdial_position = [-10.4, -28, 0];
thumbdial_radius = 4.6 + tolerance;
thumbdial_center_to_part_cutout_edge_x  = 5.34;

switch_size = [13.5, 10.2 + tolerance];
switch_position = [7.2, -28, 0];
switch_pos_offset = [5.8 - switch_position.x, -switch_size.y/2, 0];


microphone_cutout_radius = 5.25 /* measured */ + 0.2 /* extra tolerance */;
microphone_cutout_depth = 1.1 /* 423-1405-1-ND microphone datasheet */ + 0.1; /* extra tolerance */
microphone_cutout_position = [-11, 0.84, basethickness - microphone_cutout_depth]; // from center of board

thermistor_cutout_size = [3.2, 3, 0.9] /* measured */ + [0.2,0.2,0.2]; // extra tolerance
thermistor_cutout_position = [-.57, 7.52, basethickness - thermistor_cutout_size[2]]; // from center of board

usbCutoutFullThickness = false;
usb_cutout_size = [8.6, 7.1 + outeroffset, basethickness + (usbCutoutFullThickness ? boardthickness : 0)];
usb_cutout_position = [0, -46.9, 0]; // center of board to top of usb

bar_pin_cutout_distance = 6;
bar_pin_cutout_position = [0, 13, 0];
bar_pin_cutout_radius = 1;
bar_pin_cutout_depth = basethickness;

// large bar pin cutout
bar_pin_cutout_size = [25.3 /*meas*/ + 2*1.75 /*asym*/ + 1.4/*tol*/,
                        5.1 /*meas*/ + 0.6 /*tol*/,
                        basethickness];
//bar_pin_cutout_offset = [-1.75,0,0]; // bar pin is asymmetric

necklace_point_cutouts = [
                            [-11.5, 23.4, 0], [11.5, 23.4, 0],
//                            [-31.5, 19, 0], [-19, 31.5, 0],
//                            [19, 31.5, 0], [31.5, 19, 0],
                            [-8.8, -42.2, 0], [8.8, -42.2, 0],
                         ];
necklace_point_radius = 1.8;

base_cable_cutout_depth = 0.9; // my usb inner insulated strand is 0.76mm diameter
base_cable_cutout_center_position = [0, -37 - outeroffset, basethickness - base_cable_cutout_depth];
base_cable_cutout_center_radius = 4;

base_cable_spoke_1_position = [5.25, base_cable_cutout_center_position[1] + 3.9, base_cable_cutout_center_position[2]]; // from center
base_cable_spoke_2_position = [-base_cable_spoke_1_position[0], base_cable_spoke_1_position[1], base_cable_cutout_center_position[2]];
base_cable_cutout_spoke_width = 4.2;
base_cable_cutout_spoke_cap_radius = 2.1;

base_cable_stabilizer_height = 0.3;

module bottom_case() {
    translate([-1 * part_placement_offset, 0, 0]) {
        difference() {
            union() {
                // back base
                theshape(outeroffset, basethickness, inneroffset, 0, base_cutouts=true);
            
                // board groove
                theshape(outeroffset, boardthickness+extra_edge_height, 0, basethickness);
            }
            translate([0,0,-epsilon]) scale([1,1,1+epsilon]) union() {
                // microphone cutout
                translate(microphone_cutout_position)
                    cylinder(r = microphone_cutout_radius, h = microphone_cutout_depth);
                
                // thermistor cutout
                translate(thermistor_cutout_position) translate([0,0,thermistor_cutout_size[2]/2])
                    cube(thermistor_cutout_size, center=true);
                
                // usb cutout
                translate(usb_cutout_position + [-usb_cutout_size[0]/2, -usb_cutout_size[1], 0])
                    cube(usb_cutout_size);
        
                // thumbdial cutout
                translate(thumbdial_position)  {
                    cylinder(r=thumbdial_radius, h=basethickness+boardthickness + extra_edge_height);
                    translate([0,  -thumbdial_radius, 0]) cube([thumbdial_center_to_part_cutout_edge_x, 2*thumbdial_radius, basethickness]);
                }
                
                // switch cutout
                translate(switch_position + switch_pos_offset) {
                    cube([switch_size.x, switch_size.y, basethickness + boardthickness + extra_edge_height]);
                }
                
                // bar pin cutout
                translate(bar_pin_cutout_position - [bar_pin_cutout_size.x/2, bar_pin_cutout_size.y/2]) {
                    rounded_rect(bar_pin_cutout_size, 2);
                }
                
                // necklace cutouts
                for (p = necklace_point_cutouts) {
                    translate(p) cylinder(r=necklace_point_radius, h=basethickness);
                }
        
                trans_symbol(radius=4, thickness=0.2);
            }
        }
    }
}

bottom_case();
//top_case();
