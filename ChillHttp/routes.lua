function test_route(args) 
    log.info("test route");
end

return {
    {
        path="test",
        method="POST"
        handler = test_route,
    },
}

