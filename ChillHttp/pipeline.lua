
function test_func1(ctx) 
    log.trace("##### start test_func 1 #####")
    
    ctx:next();

    log.trace("##### end test_func 1 #####")
end

function test_func2(ctx) 
    log.trace("##### start test_func 2 #####")
    
    ctx:next();

    log.trace("##### end test_func 2 #####")
end

return {
    {
        id = "lua_step_0",
        handler = test_func1,
    },
    {
        id = "lua_step_1",
        handler = test_func2,
    }
};