// earthvenusmars case design

$fs = 0.01; // min angle for curved shapes
$fa = 2; // min segment/fragment size

RAD = 180 / PI;
DEG = 1;

epsilon = 0.001; // fudge to fix floating point rounding issues?
tolerance = 0.2;  // 3D printing tolerance to fit around objects

colors = ["#F15570", "white", "#2A9FFA"];

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
encased_spoke_width = abs(circle_pt(spoke_arclen/2, edge_cut_radius)[1] - circle_pt(-spoke_arclen/2, edge_cut_radius)[1]);

module theshape(outeroffset, thickness, inneroffset, zoffset, base_cutouts=false) { 
    spoke_length = 20+tolerance;
    spokes = [PI/2*RAD, 5*PI/4*RAD, 7*PI/4*RAD];
    
    difference() {
        translate([0,0,zoffset])
        union() {
            //translate([0,0,5]) cylinder(r = 20, h = thickness);
            cylinder(r = edge_cut_radius + outeroffset, h = thickness);
            for (s = [0:2]) {
                color(colors[s]) rotate(-spokes[s], [0,0,1]) translate(circle_pt(spoke_arclen/2, edge_cut_radius)) rotate(-90, [0,0,1])
                    translate([-outeroffset,0,0])
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

outeroffset = 1.8;  // max distance outside of board edge
inneroffset = -1.2; // max distance overlapping from board edge

boardthickness = 1.6 + tolerance;
bevel_max_thickness = 3;
basethickness = 1.8;
slicethickness = 0.15; // the likely minimum resolution of the printer is 0.1mm

part_placement_offset = 34;

module draw_beveled_half(flip) {
    translate([flip * part_placement_offset, 0,0]) rotate(90 + flip * 90, [0,0,1]) {
        // bevel
        slices = bevel_max_thickness * 1/slicethickness;
        for (t = [1:slices]) {
            z = bevel_max_thickness*(1 - t/slices) - slicethickness*(slices - floor(slices)) - epsilon;
            theshape(easeOutCubic(t, 0, outeroffset, slices), slicethickness, inneroffset, z);
        }
    }
};

//cutouts

thumbdial_position = [-10.4, -28.6, 0]; // centered around circular dial
thumbdial_radius = 4.6 + tolerance;
thumbdial_center_to_part_cutout_edge_x  = 5.34;

microphone_cutout_radius = 5.25 /* measured */ + 0.2 /* extra tolerance */;
microphone_cutout_depth = 1.1 /* 423-1405-1-ND microphone datasheet */ + 0.2; /* extra tolerance */
microphone_cutout_position = [-11, 0.84, basethickness - microphone_cutout_depth]; // from center of board

thermistor_cutout_size = [3.2, 3, 0.9] /* measured */ + [0.2,0.2,0.2]; // extra tolerance
thermistor_cutout_position = [-.57, 7.52, basethickness - thermistor_cutout_size[2]]; // from center of board

usb_cutout_size = [8.6, 7.1 + outeroffset, basethickness+boardthickness]; // entire thickness
usb_cutout_position = [0, -46.9, 0]; // center of board to top of usb

bar_pin_cutout_distance = 6;
bar_pin_cutout_position = [0, 13, 0];
bar_pin_cutout_radius = 1;
bar_pin_cutout_depth = boardthickness;

base_cable_cutout_depth = 0.9; // my usb inner insulated strand is 0.76mm diameter
base_cable_cutout_center_position = [0, -37 - outeroffset, basethickness - base_cable_cutout_depth];
base_cable_cutout_center_radius = 4;

base_cable_spoke_1_position = [5.25, base_cable_cutout_center_position[1] + 3.9, base_cable_cutout_center_position[2]]; // from center
base_cable_spoke_2_position = [-base_cable_spoke_1_position[0], base_cable_spoke_1_position[1], base_cable_cutout_center_position[2]];
base_cable_cutout_spoke_width = 4.2;
base_cable_cutout_spoke_cap_radius = 2.1;

base_cable_stabilizer_height = 0.3;

// top case
//draw_beveled_half(1);

// bottom case
translate([-1 * part_placement_offset, 0, 0]) {
    difference() {
        union() {
            // back base
            theshape(outeroffset, basethickness, inneroffset, 0, base_cutouts=true);
        
            // board groove
            theshape(outeroffset, boardthickness, 0, basethickness);
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
                cylinder(r=thumbdial_radius, h=basethickness);
                translate([0,  -thumbdial_radius, 0]) cube([thumbdial_center_to_part_cutout_edge_x, 2*thumbdial_radius, basethickness]);
            }
            
            // pin cutouts
            translate(bar_pin_cutout_position) {
                translate([-bar_pin_cutout_distance,0,0]) cylinder(r=bar_pin_cutout_radius, h=bar_pin_cutout_depth);
                translate([0,0,0]) cylinder(r=bar_pin_cutout_radius, h=bar_pin_cutout_depth);
                translate([bar_pin_cutout_distance,0,0]) cylinder(r=bar_pin_cutout_radius, h=bar_pin_cutout_depth);
            }

        }
    }
}
