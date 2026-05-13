/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <string>
#include <print>
#include <cstdint>
#include <stop_token>

#include <grpcpp/grpcpp.h>
#include "MSG_Position.grpc.pb.h"

#include "rotation.h"

namespace {
std::unique_ptr<grpc::Server> g_backend_grpc_server;

///////////////////////////////////////////////////////////////////////////////////////
//
// Receive message from frontend and send response with current angle
// 
//
class ReadAngleImpl final : public ReadAngleRPC::Service
{
public:
	ReadAngleImpl(rotation::driver_r208& table) :m_table(table){}

	grpc::Status SendCommand(grpc::ServerContext* context, const frontend_message* request, backend_response* response) override
	{
//#define VERBOSE__
#ifdef VERBOSE__	
		std::print("[Backend] Get frontend msg\n");
		std::print("  Source: {}\n", request->source());
		std::print("  Command: {}\n", request->command());
		std::print("  Timestamp: {} ms\n", request->timestamp_ms());
#endif

		angle_t angle{ -1.0 };

		// RUN COMMAND
		if (request->command().substr(0, 4) == "RUN=")
		{
			// Try convert
			std::string freq_string = request->command().substr(4);
			double freq = -1.0;
			try
			{
				freq = std::stod(freq_string);
			}
			catch (const std::exception& e) {
				std::print("[Backend] Invalid frequency value: '{}' Hz\n", freq_string);
				response->set_ok(false);
				return grpc::Status::OK;
			}

			std::print("[Backend] Start running\n");
			if (!m_table.start(frequency_t{ freq }))
			{
				std::print("[Backend] Failed to read angle\n");
				response->set_ok(false);
				return grpc::Status::OK;
			}

			// Init angle at 0°
			angle = 0.0;
		}

		// STOP COMMAND
		if (request->command() == "STOP")
		{
			std::print("[Backend] Stopping\n");
			if (!m_table.stop())
			{
				std::print("[Backend] Failed to stop\n");
				response->set_ok(false);
				return grpc::Status::OK;
			}
		}

		// CALIBRATE COMMAND
		if (request->command() == "CALIBRATE")
		{
			std::print("[Backend] Calibrate\n");
			if (!m_table.doCalibrating())
			{
				std::print("[Backend] Failed to calibrate\n");
				response->set_ok(false);
				return grpc::Status::OK;
			}
		}

		// HOMING COMMAND
		if (request->command() == "HOMING")
		{
			std::print("[Backend] Homing\n");
			if (!m_table.doHoming())
			{
				std::print("[Backend] Failed to home\n");
				response->set_ok(false);
				return grpc::Status::OK;
			}
		}

		// READ COMMAND
		if (request->command() == "READ")
		{
			if(!m_table.readAngle(angle))
			{
				std::print("[Backend] Failed to read angle\n");
				response->set_ok(false);
				return grpc::Status::OK;
			}
		}

		// WRITE COMMAND
		if(request->command().substr(0,6) == "WRITE=")
		{
			// Try convert
			std::string angle_string = request->command().substr(6);
			angle_t new_angle { -1.0 };
			try 
			{
				new_angle = std::stod(angle_string);
			} 
			catch (const std::exception& e) {
				std::print("[Backend] Invalid angle value: '{}' deg\n", angle_string);
				response->set_ok(false);
				return grpc::Status::OK;
			} 

			// Send back the current position
			if(!m_table.readAngle(angle))
			{
				std::print("[Backend] Failed to read angle\n");
				response->set_ok(false);
				return grpc::Status::OK;
			}

			// Apply new target angle command
			if(!m_table.setAngle(new_angle))
			{
				std::print("[Backend] Failed to set angle: '{}' deg\n", new_angle.get());
				response->set_ok(false);
				return grpc::Status::OK;
			}	
		}

		// Success
		//std::print("[Backend] Request handled successfully\n");
		response->set_rotation_angle(angle.get());
		response->set_ok(true);
		return grpc::Status::OK;
	}

private:
	rotation::driver_r208& m_table; // Reference to the rotation table driver
};
}
///////////////////////////////////////////////////////////////////////////////////////
//
// Initialize and start the gRPC server to listen for frontend messages
// 
//
uint8_t backend_rpc_init(rotation::driver_r208 &table, std::stop_token stopToken)
{
	if (stopToken.stop_requested()) return 0x0;

	std::string server_address("0.0.0.0:50051");
	ReadAngleImpl service(table);

	grpc::ServerBuilder builder;
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);

	g_backend_grpc_server = builder.BuildAndStart();

	if (!g_backend_grpc_server) {
		std::print("[Backend] Failed to start gRPC server on {}\n", server_address);
		return 0x1;
	}

	std::print("[Backend] gRPC server started on {}\n", server_address);

	// Si stop_requested() est déjà vrai à la construction → Shutdown() appelé immédiatement
	std::stop_callback stopCb(stopToken, [&]() {
		g_backend_grpc_server->Shutdown();
	});

	g_backend_grpc_server->Wait(); // Bloque jusqu'à Shutdown()

	g_backend_grpc_server.reset();
	std::print("[Backend] gRPC server stopped\n");
	return 0x0;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// Close the gRPC server
// 
//
uint8_t backend_rpc_close()
{

	return 0x0;
}

