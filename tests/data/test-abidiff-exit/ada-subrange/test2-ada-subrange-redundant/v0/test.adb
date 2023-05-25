-- Compile this file with:
--   gcc -g -c test.adb

package body Test is

function First_Function (A: My_Int_Array) return My_Int_Array is
begin
  return A;
end First_Function;


function Second_Function (A: My_Index) return My_Index is
begin
  return My_Index'Last;
end Second_Function;

end Test;
