/////////////////////////////////////////////////////////////////////////////
// Author:      Alexandre CARPENTIER
// Modified by:
// Created:     06/04/2026
// Copyright:   (c) Alexandre CARPENTIER
// Licence:     LGPL-2.1-or-later
/////////////////////////////////////////////////////////////////////////////
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QTimer>
#include <chrono>
#include <thread>

#include "frontend_rpc.h"

bool g_isReading = false;
bool g_isConnected = false;
std::jthread g_readingThread;

bool isValidAngle(std::string angle_str)
{
    if(angle_str.size() == 0)
        return false;

    double angle;
    try
    {
        angle = std::stod(angle_str);
        assert(angle >= 0 && angle <= 360);
        return true;
    }
    catch (std::exception&)
    {
        return false;
    }
}

bool isValidFreq(std::string angle_str)
{
    if (angle_str.size() == 0)
        return false;

    double angle;
    try
    {
        angle = std::stod(angle_str);
        assert(angle >= 0 && angle <= 2000);
        return true;
    }
    catch (std::exception&)
    {
        return false;
    }
}

void MainWindow::lock_controls(bool lock)
{
    if(lock)
    {
        ui->TurnBtn->setStyleSheet("background-color: #ccfca3;");
        ui->FreqEdit->setEnabled(true);

        ui->TargetAngleEdit->setEnabled(false);
        ui->CalBtn->setEnabled(false);
        ui->homeBtn->setEnabled(false);
    }
    else
    {
        ui->TurnBtn->setStyleSheet("background-color: #FFCC99;");
        ui->FreqEdit->setEnabled(false);

        ui->TargetAngleEdit->setEnabled(true);
        ui->CalBtn->setEnabled(true);
        ui->homeBtn->setEnabled(true);
    }
}

void MainWindow::status(std::string msg)
{
    ui->statusbar->showMessage(QString::fromStdString(msg));
}

void MainWindow::registersignals(Ui::MainWindow* ui)
{
    connect(ui->plusBtn, &QPushButton::clicked, this, &MainWindow::onPlusButtonClicked);
    connect(ui->minusBtn, &QPushButton::clicked, this, &MainWindow::onMinusButtonClicked);
    connect(ui->TurnBtn, &QPushButton::clicked, this, &MainWindow::onRunButtonClicked);
    connect(ui->CalBtn, &QPushButton::clicked, this, &MainWindow::onCalibrateButtonClicked);
    connect(ui->homeBtn, &QPushButton::clicked, this, &MainWindow::onHomeButtonClicked);
    connect(ui->TargetAngleEdit, &QLineEdit::editingFinished, this, &MainWindow::onTargetEditChanged);   

    connect(ui->dial, &QDial::valueChanged, this, [this, ui](int value) {
        ui->TargetAngleEdit->setText(QString::number(value, 'f', 1));
    });
    connect(ui->dial, &QDial::sliderReleased, this, &MainWindow::onDialReleased);
    connect(timer, &QTimer::timeout, this, &MainWindow::onTimerTimeout);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    lock_controls(true);
    status("Controls locked.");

    // Initialize a timer to update the angle display
    timer = new QTimer(this);

	// Initialize gRPC client
    frontend_rpc_init();
    status("gRPC client initialized.");

    // Register QT signals
    registersignals(ui);

    status("Press 'Run' to start reading the angle and enable controls.");
}

void MainWindow::onPlusButtonClicked()
{

    status("Increased");

    return;
}

void MainWindow::onMinusButtonClicked()
{

    status("Decreased");

    return;
}

void checkConnectionThread(MainWindow* obj)
{
	std::stop_token st = g_readingThread.get_stop_token();
	while (!st.stop_requested())
	{
		if (g_isConnected)
		{
			if (frontend_rpc_status() != 0)
			{
				std::print("[Frontend] gRPC client connection failed\n");
				g_isConnected = false;
                //obj->lock_controls(true);
                //obj->status("Measurement stopped.");
                //obj->ui->TurnBtn->setText("Run");

                QMetaObject::invokeMethod(
                    obj,
                    [obj]()
                    {
                        // Exécuté dans le thread principal
                        obj->lock_controls(true);
                        obj->status("Measurement stopped.");
                        obj->ui->TurnBtn->setText("Run");
                    },
                    Qt::QueuedConnection
                );

                g_readingThread.request_stop();

			}
		}
		std::this_thread::sleep_for(std::chrono::seconds(5));
	}
}

void MainWindow::onRunButtonClicked()
{
    std::string reply;
    g_isConnected = !g_isConnected;

    if (g_isConnected)
    {
        if (frontend_rpc_connect() != 0)
        {
            status("[!] gRPC client failed to connect.");
			g_isReading = false;
			g_isConnected = false;
			return;
		}

        if (frontend_rpc_status() != 0)
        {
            status("[!] gRPC client not connected.");
            g_isReading = false;
            g_isConnected = false;
            return;
        }

          // Unlock controls
        lock_controls(false);

		// Start reading
        if (!isValidFreq(ui->FreqEdit->text().toStdString()))
        {
            status("Invalid frequency value.");
			std::print("[Frontend] Invalid frequency value.\n");
            return;
        }

		double value = ui->FreqEdit->text().toDouble(); // TODO: use frequency value
		std::string command = std::format("RUN={}", value);
        if (frontend_rpc_send_command(command, reply) != 0x0)
        {
            std::print("[Frontend] Error reading angle\n");
            return;
        }

        // Check response
        if (!isValidAngle(reply))
        {
            std::print("[Frontend] Bad response from backend.\n");
			status("Invalid response from backend.");
            return;
        }

		// Set target angle = current angle
        ui->TargetAngleEdit->setText(QString::fromStdString(reply));
        ui->CurrentAngleEdit->setText(QString::fromStdString(reply));

        g_readingThread = std::jthread(checkConnectionThread, this);

        // Update every delay
        timer->start(500); 

        ui->TurnBtn->setText("Stop");
    }
    else
    {
        frontend_rpc_send_command("STOP", reply);
        lock_controls(true);

        timer->stop();
        g_readingThread.request_stop();
        //frontend_rpc_disconnect();
        g_isReading = false;
        g_isConnected = false;

        status("Measurement stopped.");
        ui->TurnBtn->setText("Run");
    }
    return;
}

void MainWindow::onCalibrateButtonClicked()
{
	std::string reply;
    if (frontend_rpc_send_command("CALIBRATE", reply) != 0x0)
    {
        std::print("[Frontend] Error CALIBRATE command\n");
        return;
    }
    return;
}

void MainWindow::onHomeButtonClicked()
{
    status("Go to home.");
    if (g_isReading)
    {
        // Send command to backend

        std::string reply;
        if (frontend_rpc_send_command("HOMING", reply) != 0x0)
        {
            std::print("[Frontend] Error calling home\n");
            return;
        }
        status("Command sent.");

        // Print initial position before rotation
        std::print("[Frontend] Read angle: \'{}\'deg\n", reply);
        ui->CurrentAngleEdit->setText(QString::fromStdString(reply));
    }
    return;
}

void MainWindow::onTargetEditChanged()
{
    status("Target changed.");
    if (g_isReading)
    {
        // Prepare command arg
        QString text = ui->TargetAngleEdit->text();
        std::print("[Frontend] Target angle set to: \'{}\'\n", text.toStdString());
        std::string command = "WRITE=" + text.toStdString();

		// Send command to backend
        
        std::string reply;
        if (frontend_rpc_send_command(command, reply) != 0x0)
        {
            std::print("[Frontend] Error writing angle\n");
            return;
        }
        status("Command sent.");

		// Print initial position before rotation
        //std::print("[Frontend] Read angle: \'{}\'deg\n", reply);
        ui->CurrentAngleEdit->setText(QString::fromStdString(reply));
    }
    return;
}

void MainWindow::onTimerTimeout()
{
    //std::print("\n[Frontend] Tick!\n" );
    status("Tick.");
    std::string reply;

    if (!g_isConnected)
    {
        timer->stop();
        std::print("[*] Timer stopped. Connection lost.\n");
        return;
    }

    if(frontend_rpc_send_command("READ", reply) != 0x0)
    {
        std::print("[Frontend] Error reading angle\n");
        status("Error reading angle.");
        return;
    }

    if(reply.size()==0)
    {
        std::print("[Frontend] No response.\n");
        status("No response.");
        return;
    }

    // Update indicators
    //std::print("[Frontend] Read angle: \'{}\'deg\n", reply);
    try
    {
		double angle = std::stod(reply);
		assert(angle >= 0 && angle <= 360);
        ui->dial->setValue(static_cast<int>(angle));
        ui->CurrentAngleEdit->setText(QString::fromStdString(reply));
    }
    catch (std::exception& e)
    {
        std::print("[Frontend] Bad response from backend.\n");
        status("Bad response.");
    }
    return;
}

void MainWindow::onDialReleased()
{
    onTargetEditChanged();
}

MainWindow::~MainWindow()
{
    delete ui;
}