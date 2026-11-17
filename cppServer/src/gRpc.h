/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include <chrono>
#include <memory>
#include <string>
#include <print>
#include <cstdint>
#include <stop_token>
#include "rotation.h"
#include <grpcpp/grpcpp.h>
#include "MSG_Position.grpc.pb.h"

inline std::unique_ptr<grpc::Server> g_backend_grpc_server;

///////////////////////////////////////////////////////////////////////////////////////
//
// Receive message from frontend and send response with current angle
// 
//
class ReadAngleImpl final : public ReadAngleRPC::Service
{
public:
	ReadAngleImpl(lexitek& table) :m_table(table) {}

	grpc::Status SendCommand(grpc::ServerContext* context, const frontend_message* request, backend_response* response) override;
private:
	positionnning_mode m_selectedMode = positionnning_mode::TIME_BASED; // Default mode
	lexitek& m_table; // Reference to the rotation table driver
};

///////////////////////////////////////////////////////////////////////////////////////
//
// Initialize and start the gRPC server to listen for frontend messages
// 
//
int backend_rpc_init(lexitek& table);

///////////////////////////////////////////////////////////////////////////////////////
//
// Close the gRPC server
// 
//
uint8_t backend_rpc_close();

