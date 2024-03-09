
function test_func(request) 
    print("##### start test_func #####")

    print(request.method);
    print(request.path);

    print("##### end test_func #####")
end

table = {};
table[1] = {
    name = "lua step",
    handler = test_func,
}

return table;