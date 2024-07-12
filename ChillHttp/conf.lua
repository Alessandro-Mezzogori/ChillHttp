-- #############################
-- ##### CHILL HTTP SERVER #####
-- #############################

-- Comment 
conf = {
    SERVING_FOLDER="D:\\Projects\\ChillHttp\\ChillHttp\\wwwroot",
    PORT="8080",
    MAX_CONCURRENT_THREADS=2,

    -- # RECV TIMEOUT IN SECONDS
    RECV_TIMEOUT=20,

    -- # Pipeline
    PIPELINE_FILE_PATH="pipeline.lua"
}

