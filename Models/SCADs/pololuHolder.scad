

//pololu
//cube([10,12,23],true);

//LONG = 23-7 = 16 14

//cover
//cube([12,14,14],true);
diam_tuerca = 6.5;
diam_tornillo = 3.4;


difference(){
 cube([14,19,16],true);   
    cube([10.5,13,23],true);
    translate([0,20,-3]) rotate([90,0,0]) cylinder(r=diam_tornillo/2,h=40,$fn=100);
    translate([0,-6.4,-3]) rotate([90,0,0]) cylinder(r=diam_tuerca/2,h=2.5,$fn=6);
    translate([0,8.9,-3]) rotate([90,0,0]) cylinder(r=diam_tuerca/2,h=2.5,$fn=6);
    //translate([0,10,0]) cube([2.7,10,12],true);
    //translate([0,-10,0]) cube([2.7,10,12],true);
}


translate([0,0,6.5])
difference(){
cube([14,34,3],true);
    
    //cube([3,29,5],true);    
    
    translate([0,13,-2]) cylinder(r=diam_tuerca/2,h=2.5,$fn=6);
    translate([0,13,-2]) cylinder(r=diam_tornillo/2,h=50,$fn=100);
    
    
    translate([0,-13,-2]) cylinder(r=diam_tuerca/2,h=2.5,$fn=6);
    translate([0,-13,-2]) cylinder(r=diam_tornillo/2,h=50,$fn=100);
    
    cube([10.5,13,23],true);
    
    
}
