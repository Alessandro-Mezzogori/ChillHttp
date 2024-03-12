function close_connection(args) 
    if(args.request.headers["Connection"] == "close") then
        args.connection:close()
    end

    pipeline.next(args);
end

function test_errors(args)
    local testError = args.request.headers["Test-Error"]

    if(testError ~= nil) then
        args.response.status = testError
    end

    pipeline.next(args)
end

function handle_request(args) 
    pipeline.handleRequest(args)
    pipeline.next(args)
end

return {
    {
        id = "close_connection_step",
        handler = close_connection,
    },
    {
        id = "test_errror",
        handler = test_errors,
    },
    {
        id = "handle_request_step",
        handler = handle_request,
    }
}

