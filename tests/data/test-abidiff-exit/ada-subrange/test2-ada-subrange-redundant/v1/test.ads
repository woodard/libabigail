package Test is

type My_Int is range 0 .. 1000;
type My_Index is range 0 .. 200;
type My_Int_Array is array (My_Index) of My_Int;

function First_Function (A: My_Int_Array) return My_Int_Array;

function Second_Function (A: My_Index) return My_Index;

end Test;