package Test1 is

type My_Int is range 0 .. 1000;
type My_Index is range 0 .. 6;
type My_Int_Array is array (My_Index) of My_Int;

function First_Function return My_Int_Array;

function Second_Function return My_Index;

end Test1;
