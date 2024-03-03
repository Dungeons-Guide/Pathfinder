#!/bin/bash

# login
aws ecr get-login-password --region us-east-1 --profile dg-profile | docker login --username AWS --password-stdin 471112686938.dkr.ecr.us-east-1.amazonaws.com

docker buildx build --platform linux/amd64,linux/arm64,linux/arm/v7 -t 471112686938.dkr.ecr.us-east-1.amazonaws.com/processor:latest --push .


