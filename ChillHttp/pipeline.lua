function test_func1(args) 
    log.info("start test_func1")
    
    print(args.request)
    print(args.request.method)
    print(args.request.path)
    print(args.request.version)
    print(args.request.body)
    print(args.request.headers)
    print(args.request.headers["Connection"])
     
    pipeline.next(args);

    log.info("end test_func1")
end

return {
    {
        id = "test_step_1",
        handler = test_func1,
    }
}

