
module fake_bear(diam_ext,diam_int,alt){
difference(){
    cylinder(r=diam_ext/2, h=alt,$fn=100);
    translate([0,0,-1])
        cylinder(r=diam_int/2, h=alt+2,$fn=100);
}
}

//Adelante (Chico)
fake_bear(12,8.4,3.5);

//Atras (Grande)
//fake_bear(18,,4);