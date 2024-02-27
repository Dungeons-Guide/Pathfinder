#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/lambda-runtime/runtime.h>
#include <iostream>
#include <memory>

#include "PathfindRequest.h"
#include "Pathfinder.h"
#include "PathfindResult.h"

using namespace aws::lambda_runtime;

char const TAG[] = "LAMBDA_ALLOC";


template <
        class result_t   = std::chrono::milliseconds,
        class clock_t    = std::chrono::steady_clock,
        class duration_t = std::chrono::milliseconds
>
auto since(std::chrono::time_point<clock_t, duration_t> const& start)
{
    return std::chrono::duration_cast<result_t>(clock_t::now() - start);
}

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}


static invocation_response my_handler(invocation_request const& req, Aws::S3::S3Client const& client)
{
    using namespace Aws::Utils::Json;
    JsonValue json(req.payload);
    if (!json.WasParseSuccessful()) {
        return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
    }

    auto v = json.View();
    if (!v.ValueExists("s3") || !v.GetObject("s3").IsObject()) {
        return invocation_response::failure("Missing event value s3", "InvalidJSON");
    }
    auto s3Data = v.GetObject("s3");
    if (!s3Data.ValueExists("bucket") || !v.GetObject("bucket").IsObject() || !v.GetObject("bucket").GetObject("name").IsString()) {
        return invocation_response::failure("Missing event value s3.bucket.name", "InvalidJSON");
    }
    auto bucket = v.GetObject("bucket").GetString("name");
    if (!s3Data.ValueExists("object") || !v.GetObject("object").IsObject() || !v.GetObject("object").GetObject("key").IsString()) {
        return invocation_response::failure("Missing event value s3.object.key", "InvalidJSON");
    }
    auto key = v.GetObject("object").GetString("key");

    AWS_LOGSTREAM_INFO(TAG, "Attempting to download file from s3://" << bucket << "/" << key);


    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(bucket).WithKey(key);

    auto outcome = client.GetObject(request);
    if (!outcome.IsSuccess()) {
        AWS_LOGSTREAM_ERROR(TAG, "Failed with error: " << outcome.GetError());
        return invocation_response::failure(outcome.GetError().GetMessage(), "Download Failure"); ;
    }

    AWS_LOGSTREAM_INFO(TAG, "Download completed!");
    auto& s = outcome.GetResult().GetBody();

    PathfindRequest pathfindRequest;
    try {
        pathfindRequest.ReadRequest(s);
    } catch (std::string &ex) {
        std::cerr << ex << std::endl;
        exit(-1);
    }


    AWS_LOGSTREAM_INFO(TAG, "ID: " << pathfindRequest.uuid << " NAME: " << pathfindRequest.name);
    AWS_LOGSTREAM_INFO(TAG, "PF ID: " << pathfindRequest.id);
    AWS_LOGSTREAM_INFO(TAG, "Target Cnt: " << pathfindRequest.target.size() << " World Size: " <<pathfindRequest.blockWorld.xLen * pathfindRequest.blockWorld.yLen * pathfindRequest.blockWorld.zLen);

    auto start = std::chrono::steady_clock::now();

    Pathfinder pathfinder(pathfindRequest);
    pathfinder.Populate();

    AWS_LOGSTREAM_INFO(TAG, "Elapsed(ms)=" << since(start).count());

    long cnt = 0;
    int x = -6;
    for (auto &item: pathfinder.nodes) {
        x++;
        int z = -6;
        for (auto &item2: item) {
            z++;
            int y = pathfinder.minY-6;
            for (auto &item3: item2) {
                y++;
                if (item3.gScore < 9999999) {
                    cnt++;
                }
            }
        }
    }
    AWS_LOGSTREAM_INFO(TAG, "Open Nodes: " << std::to_string(cnt));

    PathfindResult result;
    result.Init(pathfinder);


    Aws::S3::Model::PutObjectRequest request2;
    auto time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    request2.WithBucket(std::getenv("TARGET_BUCKET")).WithKey(key+"-"+std::to_string(time)+".pfres");

    std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::StringStream>("StringStream");
    result.WriteTo(*inputData);
    request2.SetBody(inputData);


    Aws::S3::Model::PutObjectOutcome outcome2 =
            client.PutObject(request2);
    if (!outcome2.IsSuccess()) {
        AWS_LOGSTREAM_ERROR(TAG, "Failed with error: " << outcome2.GetError());
        return invocation_response::failure(outcome.GetError().GetMessage(), "Upload Failure"); ;
    }

    AWS_LOGSTREAM_INFO(TAG, "Saved as "<< request2.GetKey() << " on " << request2.GetBucket());


    return invocation_response::success(request2.GetBucket()+"/"+request2.GetKey(), "text/plain");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
    return [] {
        return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
                "console_logger", Aws::Utils::Logging::LogLevel::Trace);
    };
}

int main()
{
    using namespace Aws;
    SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
    options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
    InitAPI(options);
    {
        Client::ClientConfiguration config;
        config.region = Aws::Environment::GetEnv("AWS_REGION");
        config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";

        S3::S3Client client(config);
        auto handler_fn = [&client](aws::lambda_runtime::invocation_request const& req) {
            return my_handler(req, client);
        };
        run_handler(handler_fn);
    }
    ShutdownAPI(options);
    return 0;
}