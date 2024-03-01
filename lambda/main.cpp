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
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/lambda-runtime/runtime.h>
#include <iostream>
#include <memory>

#include "PathfindRequest.h"
#include "Pathfinder.h"
#include "PathfindResult.h"

using namespace aws::lambda_runtime;

char const TAG[] = "PATHFINDER";


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

    if (!v.ValueExists("bucket") || !v.GetObject("bucket").IsString() || !v.ValueExists("key") || !v.GetObject("key").IsString()) {
        return invocation_response::failure("Missing key bucket or key", "InvalidJSON");
    }
    auto bucket = v.GetString("bucket");
    auto key = v.GetString("key");

    AWS_LOGSTREAM_INFO(TAG, "Attempting to download file from s3://" << bucket << "/" << key);


    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(bucket).WithKey(key);
    request.SetResponseStreamFactory(
            [=]() {
                return Aws::New<std::stringstream>("S3DOWNLOAD", std::stringstream::in | std::stringstream::out | std::stringstream::binary);
            }
    );

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
        AWS_LOGSTREAM_ERROR(TAG, "Parsing failed with error: " << ex);
        return invocation_response::failure(ex, "Parsing Failure"); ;
    }


    AWS_LOGSTREAM_INFO(TAG, "ID: " << pathfindRequest.uuid << " NAME: " << pathfindRequest.name);
    AWS_LOGSTREAM_INFO(TAG, "PF ID: " << pathfindRequest.id);
    AWS_LOGSTREAM_INFO(TAG, "Target Cnt: " << pathfindRequest.target.size() << " World Size: " <<pathfindRequest.blockWorld.xLen * pathfindRequest.blockWorld.yLen * pathfindRequest.blockWorld.zLen << " Node Size: " << (
            pathfindRequest.octNodeWorld.xLen * pathfindRequest.octNodeWorld.yLen * pathfindRequest.octNodeWorld.zLen));

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

    std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::StringStream>("StringStream", std::stringstream::in | std::stringstream::out | std::stringstream::binary);
    result.WriteTo(*inputData);
    request2.SetBody(inputData);


    Aws::S3::Model::PutObjectOutcome outcome2 =
            client.PutObject(request2);
    if (!outcome2.IsSuccess()) {
        AWS_LOGSTREAM_ERROR(TAG, "Failed with error: " << outcome2.GetError());
        return invocation_response::failure(outcome.GetError().GetMessage(), "Upload Failure"); ;
    }

    AWS_LOGSTREAM_INFO(TAG, "Saved as "<< request2.GetKey() << " on " << request2.GetBucket());

    Aws::S3::Model::DeleteObjectRequest request3;
    request3.WithBucket(bucket).WithKey(key);
    Aws::S3::Model::DeleteObjectOutcome outcome3 = client.DeleteObject(request3);
    if (!outcome3.IsSuccess()) {
        AWS_LOGSTREAM_ERROR(TAG, "Failed to cleanup s3 file: " << request3.GetBucket()<< "/"<< request3.GetKey());
    }

    return invocation_response::success(request2.GetBucket()+"/"+request2.GetKey(), "text/plain");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
    return [] {
        return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
                "console_logger", Aws::Utils::Logging::LogLevel::Info);
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