#include "debuggerwindow.h"
#include "ui_debuggerwindow.h"
#include <string>
#include <GBCEmulator.h>

DebuggerWindow::DebuggerWindow(QWidget *parent)
    :   QMainWindow(parent),
        ui(new Ui::DebuggerWindow),
        hexWidget(new HexWidget(parent))
{
    ui->setupUi(this);
    ui->scrollArea->setWidget(hexWidget);
    connectToolbarButtons();
    this->show();
}

DebuggerWindow::DebuggerWindow(QWidget *parent, std::shared_ptr<EmuView> emu)
    :   QMainWindow(parent),
        ui(new Ui::DebuggerWindow),
        hexWidget(new HexWidget(parent)),
        emuView(emu)
{
    ui->setupUi(this);
    ui->scrollArea->setWidget(hexWidget);
    ui->graphicsView->setScene(emuView->getThis());
    connectToolbarButtons();

    if (emuView->emu)
    {
        emuView->emu->debugMode = true;
        initEmulatorConnections(emuView->emu);
    }

    connect(this, &DebuggerWindow::runEmulator, emuView.get(), &EmuView::runEmulator);
    connect(this, &DebuggerWindow::runTo, emuView.get(), &EmuView::runTo);

    this->show();
}

DebuggerWindow::~DebuggerWindow()
{
    if (cpu)
    {
        cpu.reset();
    }

    if (emu)
    {
        emu.reset();
    }

    if (emuView)
    {
        emuView.reset();
    }
    
    if (hexWidget)
    {
        delete hexWidget;
    }

    delete ui;
}

void DebuggerWindow::initEmulatorConnections(std::shared_ptr<GBCEmulator> _emu)
{
    emu = _emu;

    if (emu)
    {
        cpu = emu->get_CPU();
        updateHexWidget();
        hexWidget->setCursor(cpu->get_register_16(CPU::PC));
    }

    // Setup updateGUITimer to update GUI when the emulator is running
    connect(&updateGUITimer, &QTimer::timeout, [&]()
    {
        if (cpu && emu->ranInstruction)
        {
            emu->ranInstruction = false;
            updateRegisterLabels();
            updateHexWidget();
            hexWidget->setCursor(cpu->get_register_16(CPU::PC));
        }
    });
    updateGUITimer.start(1);
}

void DebuggerWindow::connectToolbarButtons()
{
    // Connect Continue button
    connect(ui->actionContinue, &QAction::triggered, [&]()
    {
        ui->actionContinue->setChecked(true);
        ui->actionPause->setChecked(false);
        emu->debugMode = false;
        emit runEmulator();
    });

    // Connect Pause button
    connect(ui->actionPause, &QAction::triggered, [&]()
    {
        ui->actionContinue->setChecked(false);
        ui->actionPause->setChecked(true);
        emu->debugMode = true;
        emu->stop();

        pc = cpu->get_register_16(CPU::REGISTERS::PC);
        next_pc = pc + cpu->getInstructionSize(cpu->getByteFromMemory(pc));
    });

    // Connect Step Into button
    connect(ui->actionStep_Into, &QAction::triggered, [&]()
    {
        ui->actionContinue->setChecked(false);
        emu->setStopRunning(false);

        pc = cpu->get_register_16(CPU::REGISTERS::PC);
        emu->runNextInstruction();
    });

    // Connect Step Over button
    connect(ui->actionStep_Over, &QAction::triggered, [&]()
    {
        ui->actionContinue->setChecked(false);
        emu->setStopRunning(false);

        pc = cpu->get_register_16(CPU::REGISTERS::PC);
        next_pc = pc + cpu->getInstructionSize(cpu->getByteFromMemory(pc));

        emit runTo(next_pc);
    });

    // Connect Step Out button
    connect(ui->actionStep_Out, &QAction::triggered, [&]()
    {
        ui->actionContinue->setChecked(false);
        emu->setStopRunning(false);

        emit runTo(next_pc);
    });

    // Connect Run To button
    connect(ui->pushButton_RunTo, &QPushButton::pressed, [&]()
    {
        uint16_t val_16;
        QString val = ui->lineEdit_RunTo->text();
        bool isInt = false;
        bool isHex = false;
        val = val.toLower();
        if (val.startsWith("0x") && val.size() <= 6)
        {   // Parse hex value to number
            val = val.mid(2, val.size());
            isHex = true;
        }

        if (isHex)
        {
            val_16 = val.toInt(&isInt, 16);
        }
        else
        {
            val_16 = val.toInt(&isInt, 10);
        }

        if (isInt)
        {
            emit runTo(val_16);
        }
        else
        {
            ui->lineEdit_RunTo->setText("Invalid position!");
        }
    });
}

void DebuggerWindow::updateRegisterLabels()
{
    ui->label_AF_Value->setText("0x" + makeQStringHex(cpu->get_register_16(CPU::AF)));
    ui->label_BC_Value->setText("0x" + makeQStringHex(cpu->get_register_16(CPU::BC)));
    ui->label_DE_Value->setText("0x" + makeQStringHex(cpu->get_register_16(CPU::DE)));
    ui->label_HL_Value->setText("0x" + makeQStringHex(cpu->get_register_16(CPU::HL)));
    ui->label_PC_Value->setText("0x" + makeQStringHex(cpu->get_register_16(CPU::PC)));
    ui->label_SP_Value->setText("0x" + makeQStringHex(cpu->get_register_16(CPU::SP)));
    
    uint8_t instruc = cpu->getByteFromMemory(CPU::PC);
    QString intrucTrans = QString::fromStdString(cpu->getOpcodeString(instruc));
    ui->label_Instruction_Value->setText("0x" + makeQStringHex(instruc, 2) + " : " + intrucTrans);
}

void DebuggerWindow::updateHexWidget()
{
    const std::vector<uint8_t> memoryMap = emu->get_memory_map();
    hexWidget->setData(memoryMap);
}

template<typename T>
QString DebuggerWindow::makeQStringHex(T number, int num_digits)
{
    return QString::number(number, 16).rightJustified(num_digits, '0').toUpper();
}