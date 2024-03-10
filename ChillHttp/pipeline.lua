
--function test_func(request) 
--    print("##### start test_func #####")
--
--    print(request.method);
--    print(request.path);
--
--    print("##### end test_func #####")
--end

a = array.new(1000)
for i = 1, 1000 do
    a[i] = i % 2 == 0 -- a[i] = (i % 2 == 0)
end
print(a[10]) --> true
print(a[11]) --> false
print(#a) --> 1000

table = {};
--table[1] = {
--    name = "lua step",
--    handler = test_func,
--}
return table;