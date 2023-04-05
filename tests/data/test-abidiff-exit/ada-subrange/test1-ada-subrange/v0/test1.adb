-- Compile this file with:
--   gcc -g -c test1.adb

package body Test1 is

function First_Function return My_Int_Array is
  A : My_Int_Array;
begin
  A := (1,2,3,4,5,6);
  return A;
end First_Function;


function Second_Function return My_Index is
begin
  return My_Index'Last;
end Second_Function;

end Test1;
