/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include <string>
#include <chrono>
#include <print>

#include <grpcpp/grpcpp.h>
#include "MSG_Position.grpc.pb.h"

std::unique_ptr<ReadAngleRPC::Stub> grpc_stub;
std::shared_ptr<grpc::Channel> g_channel;

int frontend_rpc_init()
{
    g_channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());

    grpc_stub = ReadAngleRPC::NewStub(g_channel);
    std::print("[Frontend] Client gRPC initialized (localhost:50051)\n");
    return 0x0;
}

int frontend_rpc_status()
{
    grpc_connectivity_state state = g_channel->GetState(true);

    if (state != GRPC_CHANNEL_READY)
    {
        std::print("[Frontend] Client gRPC not connected\n");
        return -1;
    }

    std::print("[Frontend] Client gRPC connected\n");
    return 0;
}

int frontend_rpc_connect()
{
    // Timeout
    auto deadline = std::chrono::system_clock::now()
        + std::chrono::seconds(1);

    if (!g_channel->WaitForConnected(deadline))
    {
		std::print("[Frontend] Client gRPC connection failed\n");
		return -1;
    }

    std::print("[Frontend] Client gRPC connection success\n");
    return 0;
}

int frontend_rpc_disconnect()
{
    g_channel.reset();

    std::print("[Frontend] Client gRPC disconnected\n");
    return 0;
}


int frontend_rpc_send_command(const std::string& command, std::string& response)
{
    frontend_message request;
    request.set_source("frontend");
    request.set_command(command);
    request.set_timestamp_ms(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    backend_response ack;
    grpc::ClientContext context;

    grpc::Status status = grpc_stub->SendCommand(&context, request, &ack);

    if (status.ok())
    {
//#define VERBOSE__
#ifdef VERBOSE__	
        std::print("[Frontend] Rotating.\n");
        std::print("[Frontend] Read angle from backend: \'{}\'deg\n", ack.rotation_angle());
#endif
        response = std::format("{:.1f}", ack.rotation_angle());
    }
    else
    {
        std::print("[Frontend] gRPC error: {}\n", status.error_message());
        std::print("[last command]: {}\n", request.command());
        //QMessageBox::warning(nullptr, "gRPC Error",
        //    QString::fromStdString(status.error_message()));
    }
	return 0x0;
}

int frontend_rpc_run(double frequency, std::string& response)
{
    run run_request;
    run_request.set_source("frontend run");
	run_request.set_frequency(frequency);
    run_request.set_timestamp_ms(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );

    backend_response ack;
    grpc::ClientContext context;

    grpc::Status status = grpc_stub->StartCommand(&context, run_request, &ack);

    if (status.ok())
    {
        //#define VERBOSE__
#ifdef VERBOSE__	
        std::print("[Frontend] Rotating.\n");
        std::print("[Frontend] Read angle from backend: \'{}\'deg\n", ack.rotation_angle());
#endif
        response = std::format("{:.1f}", ack.rotation_angle());
    }
    else
    {
        std::print("[Frontend] Erreur gRPC: {}\n", status.error_message());
    }
    return 0x0;
}

int frontend_rpc_close()
{
    std::print("[Frontend] Closing gRPC client\n");
    if(grpc_stub)
    {
        grpc_stub.reset();
    }
    return 0x0;
}


