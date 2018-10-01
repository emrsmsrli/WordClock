$fn=50;
materialWidth = 2;

numberOfSlots = 10;
spacingOfSlots = 16.5; //Absolute, between centres, this is the LED spacing

cableSlot = 5; // Width/height of cable slot

shortLength = 37;
shortHeight = 22;

slatLength = 231; //Overall length
slatHeight = 30; //Overall height

backPlaneLength = 231;
lockingPinWidth = 5;
lockingPinRafterInset = 10;

slotWidth = materialWidth+0.1; //Slot plus a little for fitting
slotHeight = slatHeight/2+0.5; //height plus a little for fitting


spaceBetweenRepeats = 6; //When you repeat them

shiftBy = (slatLength-((numberOfSlots-1)*spacingOfSlots+slotWidth))/2;


//slatLength/2 -
//        (numberOfSlots*(spacingOfSlots+slotWidth))/2 + spacingOfSlots/2;

for (i = [0:0]){
    translate([0,i*(slatHeight+spaceBetweenRepeats)])
        rafter(cableHole = "yes", lockingPin ="yes");
    }

for (i = [1:1]){
    translate([0,i*(slatHeight+spaceBetweenRepeats)])
        rafter(cableHole = "no", short ="yes");
    }


    
for (i = [2:2]){
    translate([0,i*(slatHeight+spaceBetweenRepeats)])
        translate([0,0]){
            backPlate();}
    }    
    
    


module rafter (){
    //one of them
            if(lockingPin=="yes"){
            translate([lockingPinRafterInset,slatHeight]){
                square(size = [lockingPinWidth,materialWidth],center=false);
            }
            translate([slatLength-lockingPinRafterInset-lockingPinWidth,slatHeight]){
                square(size = [lockingPinWidth,materialWidth],center=false);
            }
        }
    difference() {
        square(thickness = materialWidth, size = [slatLength,slatHeight],center=false); //main slat
        if(cableHole=="yes"){
            //power cable
            translate([shiftBy/2
                    ,slatHeight-cableSlot])
            square(size = [cableSlot,cableSlot],center=false);}


            //short
        if(short=="yes"){
            square(size = [shortLength,shortHeight],center=false);}
        

        
        //Move the slots to centre them
        translate([shiftBy
                ,0,0])
            //square(size = [5,5], center = false);
        
           for (i = [0:numberOfSlots-1]){
                translate([i*(spacingOfSlots),0])
                square(size = [slotWidth,slotHeight],center=false); //removed slat
        }
        
    }
}



module backPlate(){
    difference() {
        square(thickness = materialWidth, size = [backPlaneLength,backPlaneLength],center=false); 
        translate([backPlaneLength-80-3,3]){ //indent the pcb from the edge by this much

                
            circuitBoard();
        }
        translate([backPlaneLength/2 - 6/2,0]){
                square(size=[6,6]);
        }
        
        translate([0,((backPlaneLength-((numberOfSlots-1)*spacingOfSlots+slotWidth))/2)]){
            for (i = [0:numberOfSlots-1]){
                translate([0,i*spacingOfSlots]){
                    translate([lockingPinRafterInset+(backPlaneLength-slatLength)/2,0]){ //left side
                        square([lockingPinWidth+0.1,materialWidth+0.1]); //slot for positioning
                    }
                    translate([backPlaneLength-lockingPinWidth-lockingPinRafterInset-(backPlaneLength-slatLength)/2,0]){ //right side
                        square([lockingPinWidth+0.1,materialWidth+0.1]); //slot for positioning
                    
                    }
                }
            }
        }
        
        
    }
}


module circuitBoard(){
            //button 1
            standOffHoleSize = 3/2; //radius
            buttonHoleSize = 4/2;//radius

            translate([75-11.43,25.65]){ //80-18.221,35-28.656
                circle(buttonHoleSize);
            }
            //button 2
            translate([75-21.59,25.65]){ //80-38.481,35-28.656
                circle(buttonHoleSize);
            }
            
            //standoffs x4
             translate([3,3]){
                circle(standOffHoleSize);
            }       
             translate([3,27]){
                circle(standOffHoleSize);
            }  
            translate([72,3]){
                circle(standOffHoleSize);
            }  
            translate([72,27]){
                circle(standOffHoleSize);
            } 

            //FTDI header
            translate([75-53.34-2,10.6]){
                square(size=[4,20], center=true);
            }

}

module pcbAdapter(){
    difference(){
        circle(6/2);
        circle(4/2);
    }
}