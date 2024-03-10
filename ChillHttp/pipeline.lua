

function test_func1(args) 
    log.trace("##### start test_func 1 #####")
    
    print("routes len %d", #routes)
    print(routes[1])
    print(routes[1].path)
    print(routes[1].method)
    
    print("port " .. config.port)
    print("max concurrent threads " .. tostring(config.maxConcurrentThreads))

    -- pipeline.next(args);

    log.trace("##### end test_func 1 #####")
end

function handle_request_func(args) 
    log.trace("##### start handle_request_func #####")
    
    pipeline.next(args);
    pipeline.handleRequest(args);
    
    log.trace("##### end handle_request_func #####")
end

return {
    {
        id = "test_step_1",
        handler = test_func1,
    },
--    {
--        id = "handle_request",
--        handler = handle_request_func,
--    }
};