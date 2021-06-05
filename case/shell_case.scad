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

outeroffset = 2.2;  // max distance outside of board edge
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

pcb_bottom_y = 52.79; // measured board center to pcb bottom y
pcb_bottom_to_dial_center = 26.4;
thumbdial_radius = 4.6 + tolerance;
thumbdial_center_to_edge_x = 1.3;

//cutouts

microphone_cutout_radius = 4.43 /* measured */ + 0.6 /* tolerance */;
microphone_cutout_depth = 1.5; // 0805 1uF capacitor seems to be tallest part at 1.4mm expected
microphone_cutout_position = [-10.65, 1.28, basethickness - microphone_cutout_depth]; // from center of board

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
    // cable stabilizers
    for(y = [0:2:16])
    translate(base_cable_cutout_center_position+[base_cable_cutout_center_radius,-y,0])
        rotate([0,-PI/2*RAD,0])
            linear_extrude(base_cable_cutout_center_radius*2)
                polygon([[0,0], [0, 3/2*base_cable_stabilizer_height], [base_cable_stabilizer_height, 1/2*base_cable_stabilizer_height]]);


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
            
            // wire cutout
            translate(base_cable_cutout_center_position) {
                cylinder(r=base_cable_cutout_center_radius, h=base_cable_cutout_depth);
            }
            translate(base_cable_spoke_1_position)
                cylinder(r=base_cable_cutout_spoke_cap_radius, h=base_cable_cutout_depth);
            translate(base_cable_spoke_2_position)
                cylinder(r=base_cable_cutout_spoke_cap_radius, h=base_cable_cutout_depth);
            
            spoke_1_rect_pos = (base_cable_spoke_1_position + base_cable_cutout_center_position) / 2;
            spoke_1_angle = angle(base_cable_spoke_1_position-base_cable_cutout_center_position, [1,0,0]);
            translate(spoke_1_rect_pos + [0,0,base_cable_cutout_depth/2])
                rotate([0,0,spoke_1_angle])
                    cube([norm(base_cable_spoke_1_position - base_cable_cutout_center_position), base_cable_cutout_spoke_width, base_cable_cutout_depth], center=true);
            spoke_2_rect_pos = (base_cable_spoke_2_position + base_cable_cutout_center_position) / 2;
            spoke_2_angle = angle(base_cable_spoke_2_position-base_cable_cutout_center_position, [1,0,0]);
            translate(spoke_2_rect_pos + [0,0,base_cable_cutout_depth/2])
                rotate([0,0,spoke_2_angle])
                    cube([norm(base_cable_spoke_2_position - base_cable_cutout_center_position), base_cable_cutout_spoke_width, base_cable_cutout_depth], center=true);
            // wire route out of board
            wire_length = 20; // long enough to exit board
            translate(base_cable_cutout_center_position + [-base_cable_cutout_spoke_width/2,0,0])
                scale([1,-1,1]) cube([base_cable_cutout_spoke_width, wire_length, base_cable_cutout_depth+boardthickness]);
            
            // thumbdial cutout
            translate([-encased_spoke_width/2 + thumbdial_center_to_edge_x, -pcb_bottom_y + pcb_bottom_to_dial_center, 0])  {
                cylinder(r=thumbdial_radius, h=basethickness);
                translate([0,  -thumbdial_radius, 0]) cube([2*thumbdial_radius, 2*thumbdial_radius, basethickness]);
            }
        }
    }
}
