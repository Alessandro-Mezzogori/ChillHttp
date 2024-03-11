function test_func1(args) 
    log.info("start test_func1")
    
    print(args.response)
    args.response.version = 2
    args.response.statusCode = 403
    args.response.body = "Forbidden Test"
    args.response.headers["Content-Type"] = "text/plain"
     
    pipeline.next(args);

    log.info("end test_func1")
end

return {
    {
        id = "test_step_1",
        handler = test_func1,
    }
}

