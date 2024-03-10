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
#include <iostream>
#include <memory>

#include "PathfindRequest.h"
#include "Pathfinder.h"
#include "PathfindResult.h"


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


int main()
{
    using namespace Aws;
    SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    options.httpOptions.installSigPipeHandler = true;
    InitAPI(options);
    
    Client::ClientConfiguration config;
    config.region = Aws::Environment::GetEnv("AWS_REGION");
    config.caFile = "/etc/ssl/certs/ca-certificates.crt";

    S3::S3Client client(config);
        
    auto bucket = std::string (std::getenv("SOURCE_BUCKET"));
    auto key = std::string(std::getenv("SOURCE_KEY"));

    std::cout << "Attempting to download file from s3://" << bucket << "/" << key << std::endl;


    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(bucket).WithKey(key);
    request.SetResponseStreamFactory(
            [=]() {
                return Aws::New<std::stringstream>("S3DOWNLOAD", std::stringstream::in | std::stringstream::out | std::stringstream::binary);
            }
    );

    auto outcome = client.GetObject(request);
    if (!outcome.IsSuccess()) {
        std::cerr << "Download Failure: Failed with error: " << outcome.GetError() << std::endl;
        exit(-1);
        return -1;
    }

    std::cout << "Download completed!";
    auto& s = outcome.GetResult().GetBody();

    PathfindRequest pathfindRequest;
    try {
        pathfindRequest.ReadRequest(s);
    } catch (std::string &ex) {
        std::cerr << "Parsing failed with error: " << ex << std::endl;
        exit(-1);
        return -1;
    }


    std::cout << "ID: " << pathfindRequest.uuid << " NAME: " << pathfindRequest.name << std::endl;
    std::cout << "PF ID: " << pathfindRequest.id << std::endl;
    std::cout << "Target Cnt: " << pathfindRequest.target.size() << " World Size: " <<pathfindRequest.blockWorld.xLen * pathfindRequest.blockWorld.yLen * pathfindRequest.blockWorld.zLen << " Node Size: " << (
            pathfindRequest.octNodeWorld.xLen * pathfindRequest.octNodeWorld.yLen * pathfindRequest.octNodeWorld.zLen) << std::endl;

    auto start = std::chrono::steady_clock::now();

    Pathfinder pathfinder(pathfindRequest);
    std::cout << "Min" << pathfinder.minY << " Max " << pathfinder.maxY << std::endl;
    pathfinder.Populate();
    std::cout << "Elapsed(ms)=" << since(start).count() << std::endl;

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
    std::cout << "Open Nodes: " << std::to_string(cnt) << std::endl;

    PathfindResult result;
    result.Init(pathfinder);


    int runcnt = 0;
    auto time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    while(true) {
        runcnt++;
        Aws::S3::Model::PutObjectRequest request2;
        request2.WithBucket(std::getenv("TARGET_BUCKET")).WithKey(std::getenv("TARGET_KEY"));

        std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::StringStream>("StringStream", std::stringstream::in | std::stringstream::out | std::stringstream::binary);
        result.WriteTo(*inputData);
        request2.SetBody(inputData);


        Aws::S3::Model::PutObjectOutcome outcome2 =
                client.PutObject(request2);
        if (!outcome2.IsSuccess()) {
            std::cerr << "Upload Failed with error: " << outcome2.GetError() << std::endl;
            if (runcnt > 5) {
                exit(-1);
                return -1;
            }
            continue;
        }

        std::cout << "Saved as "<< request2.GetKey() << " on " << request2.GetBucket() << std::endl;
        break;
    }

    Aws::S3::Model::DeleteObjectRequest request3;
    request3.WithBucket(bucket).WithKey(key);
    Aws::S3::Model::DeleteObjectOutcome outcome3 = client.DeleteObject(request3);
    if (!outcome3.IsSuccess()) {
        std::cerr << "Failed to cleanup s3 file: " << request3.GetBucket()<< "/"<< request3.GetKey() << std::endl;
    }

    ShutdownAPI(options);
    return 0;
}