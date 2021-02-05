// earthvenusmars case design

$fs = 0.01; // min angle for curved shapes
$fa = 2; // min segment/fragment size

RAD = 180 / PI;
DEG = 1;

epsilon = 0.001; // fudge to fix floating point rounding issues?

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
encased_radius = 24;
spoke_arclen = PI/3*RAD;
encased_spoke_width = abs(circle_pt(spoke_arclen/2, encased_radius)[1] - circle_pt(-spoke_arclen/2, encased_radius)[1]);

module theshape(outeroffset, thickness, inneroffset, zoffset) { 
    spoke_length = 20;
    spokes = [PI/2*RAD, 5*PI/4*RAD, 7*PI/4*RAD];
    
    difference() {
        translate([0,0,zoffset])
        union() {
            cylinder(r = encased_radius + outeroffset, h = thickness);
            for (s = [0:2]) {
                color(colors[s]) rotate(-spokes[s], [0,0,1]) translate(circle_pt(spoke_arclen/2, encased_radius)) rotate(-90, [0,0,1])
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
        
        translate([0,0,zoffset])
        translate([0,0,-epsilon]) union() {
            cylinder(r = encased_radius + inneroffset, h = thickness+2*epsilon);
            for (s = [0:2]) {
                color(colors[s]) rotate(-spokes[s], [0,0,1]) translate(circle_pt(spoke_arclen/2, encased_radius)) rotate(-90, [0,0,1])
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

outeroffset = 2;  // max distance outside of board edge
inneroffset = -1; // max distance overlapping from board edge

chunkthickness = 1.7; // 1/3 total thickness. board is 1.6mm thick
slicethickness = 0.05; // the likely minimum resolution of the printer is 0.1mm

placement_offset = 30;
module draw_beveled_half(flip) {
    translate([flip * placement_offset, 0,0]) rotate(90 + flip * 90, [0,0,1]) {
        // board groove
        zaccum = chunkthickness/2;
        theshape(outeroffset, zaccum, 0, 0);
        
        // bevel
        slices = chunkthickness * 1/slicethickness;
        for (t = [1:slices]) {
            z = zaccum + chunkthickness*(1 - t/slices);
            theshape(easeOutCubic(t, 0, outeroffset, slices), slicethickness, inneroffset, z);
        }
    }
};

pcb_bottom_y = 51.3825;
pcb_bottom_to_dial = 19.7;
thumbdial_base_width = 7.62;
thumbdial_center_to_edge_x = 1.785;
// top half of case
draw_beveled_half(1);
difference() {
    draw_beveled_half(-1);
    
    // thumbdial cutout
    translate([-placement_offset + encased_spoke_width/2 - thumbdial_center_to_edge_x, -pcb_bottom_y + pcb_bottom_to_dial + thumbdial_base_width/2, chunkthickness/2]) 
    cylinder(r=thumbdial_base_width/2, h=chunkthickness+epsilon);

}
