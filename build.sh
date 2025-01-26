#!/bin/bash

# login
aws ecr get-login-password --region us-east-1 --profile dg-profile | docker login --username AWS --password-stdin 471112686938.dkr.ecr.us-east-1.amazonaws.com

docker buildx build --platform linux/arm64,linux/amd64 -t 471112686938.dkr.ecr.us-east-1.amazonaws.com/processor:2.0 --push .


