RAD = 180 / PI;
DEG = 1;

$fs = 0.01; // min angle for curved shapes
$fa = 2; // min segment/fragment size


epsilon = 0.001;

module peg(socket=false) {
    peg_dim = [0.4, 3, 1];
    notch_size = 0.35;
    notch_width = notch_size/2;
    notch_dim = [notch_size, peg_dim[1], notch_size];
    
    if (socket) {        
        socket_extension = notch_width;
        translate([-socket_extension, 0,0]) scale([1+epsilon, 1, 1]) 
        difference() {
            cube([socket_extension, peg_dim[1], peg_dim[2]]);
            //bevel top outside corner on socket (which is offset, hence the duplication)
            //color("red") translate([-.06, peg_dim[1]/2-epsilon, peg_dim[2]]) rotate(3*PI/16*RAD, [0,1,0]) scale([1, 1+epsilon, 1])
            //    cube([.4, peg_dim[1], .4], center=true);
        }
    }
    
    // peg & notch
    difference() {
        union() {
            cube(peg_dim);
            translate([peg_dim[0] - epsilon, 0, peg_dim[2]-notch_dim[2]]) cube(notch_dim);
        }
        union() {
            translate([peg_dim[0], -epsilon, peg_dim[2]]) rotate(PI/6*RAD, [0,1,0]) scale([1,1+epsilon,1]) cube([1,notch_dim[1],1]);
            // bevel top outside corner on peg
            if (!socket) translate([-.06, peg_dim[1]/2-epsilon, peg_dim[2]]) rotate(3*PI/16*RAD, [0,1,0]) scale([1, 1+epsilon, 1])
                cube([.4, peg_dim[1], .4], center=true);
        }
    }
}

peg();
translate([5, 0, 0]) difference() {
    translate([-2, -1,0]) cube([3,3, 3]);
    translate([0,0,-epsilon]) peg(socket=true);
    
};
//color("blue") translate([5, 0, 0]) peg();


