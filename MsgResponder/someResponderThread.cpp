/*==============================================================================
==============================================================================*/

//******************************************************************************
//******************************************************************************
//******************************************************************************
#include "stdafx.h"

#include "someSerialParms.h"

#define  _SOMERESPONDERTHREAD_CPP_
#include "someResponderThread.h"

namespace Some
{

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Constructor.

ResponderThread::ResponderThread()
{
   // Set base class variables.
   BaseClass::setThreadName("Responder");
   BaseClass::setThreadPriority(Ris::Threads::gPriorities.mProc);
   BaseClass::mTimerPeriod = 100;

   // Initialize qcalls.
   mSessionQCall.bind(this, &ResponderThread::executeSession);
   mRxMsgQCall.bind   (this,&ResponderThread::executeRxMsg);
   mAbortQCall.bind(this, &ResponderThread::executeAbort);

   // Initialize member variables.
   mSerialMsgThread = 0;
   mMsgMonkey = new RGB::MsgMonkey;
   mConnectionFlag = false;
   mTPFlag = true;
   mRxCount = 0;
   mTxCount = 0;
   mShowCode = 0;
}

ResponderThread::~ResponderThread()
{
   if (mSerialMsgThread) delete mSerialMsgThread;
   delete mMsgMonkey;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Show thread info for this thread and for child threads.

void ResponderThread::showThreadInfo()
{
   BaseClass::showThreadInfo();
   if (mSerialMsgThread)
   {
      mSerialMsgThread->showThreadInfo();
   }
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Thread init function. This is called by the base class immedidately 
// after the thread starts running. It creates and launches the 
// child SerialMsgThread.

void ResponderThread::threadInitFunction()
{
   Trc::write(11, 0, "ResponderThread::threadInitFunction");

   // Instance of serial port settings.
   Ris::SerialSettings tSerialSettings;

   tSerialSettings.setPortDevice(gSerialParms.mSerialPortDevice);
   tSerialSettings.setPortSetup(gSerialParms.mSerialPortSetup);
   tSerialSettings.mThreadPriority = Ris::Threads::gPriorities.mSerial;
   tSerialSettings.mRxTimeout = gSerialParms.mSerialRxTimeout;
   tSerialSettings.mMsgMonkey = mMsgMonkey;
   tSerialSettings.mSessionQCall = mSessionQCall;
   tSerialSettings.mRxMsgQCall = mRxMsgQCall;
   tSerialSettings.mTraceIndex = 11;
   Trc::start(11);

   // Create child thread with the settings.
   mSerialMsgThread = new Ris::SerialMsgThread(tSerialSettings);

   // Launch child thread.
   mSerialMsgThread->launchThread(); 

   Trc::write(11, 0, "ResponderThread::threadInitFunction done");
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Thread exit function. This is called by the base class immedidately
// before the thread is terminated. It shuts down the child SerialMsgThread.

void ResponderThread::threadExitFunction()
{
   Trc::write(11, 0, "ResponderThread::threadExitFunction");
   Prn::print(0, "ResponderThread::threadExitFunction BEGIN");

   // Shutdown the child thread.
   mSerialMsgThread->shutdownThread();

   Prn::print(0, "ResponderThread::threadExitFunction END");
   Trc::write(11, 0, "ResponderThread::threadExitFunction done");
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Thread shutdown function. This calls the base class shutdownThread
// function to terminate the thread. This executes in the context of
// the calling thread.

void ResponderThread::shutdownThread()
{
   Trc::write(11, 0, "ResponderThread::shutdownThread");
   Prn::print(0, "ResponderThread::shutdownThread BEGIN");
   BaseClass::shutdownThread();
   Prn::print(0, "ResponderThread::shutdownThread END");
   Trc::write(11, 0, "ResponderThread::shutdownThread done");
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Execute periodically. This is called by the base class timer.

void ResponderThread::executeOnTimer(int aTimerCount)
{
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Abort function. This is bound to the qcall. It aborts the serial port.

void ResponderThread::executeAbort()
{
   mSerialMsgThread->mSerialMsgPort.doAbort();
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// qcall registered to the mSerialMsgThread child thread. It is invoked
// when a session is established or disestablished (when the serial port
// is opened or it is closed because of an error or a disconnection). 

void ResponderThread::executeSession(bool aConnected)
{
   if (aConnected)
   {
      Prn::print(Prn::Show1, "ResponderThread CONNECTED");
   }
   else
   {
      Prn::print(Prn::Show1, "ResponderThread DISCONNECTED");
   }

   mConnectionFlag = aConnected;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// qcall registered to the mSerialMsgThread child thread. It is invoked by
// the child thread when a message is received.
// Based on the receive message type, call one of the specific receive
// message handlers. This is bound to the qcall.

void ResponderThread::executeRxMsg(Ris::ByteContent* aRxMsg)
{
   RGB::BaseMsg* tRxMsg = (RGB::BaseMsg*)aRxMsg;

   // Test flag to ignore messages.
   if (!mTPFlag)
   {
      delete tRxMsg;
      return;
   }

   // Message jump table based on message type.
   // Call corresponding specfic message handler method.
   switch (tRxMsg->mMessageType)
   {
   case RGB::MsgIdT::cTestMsg:
      processRxMsg((RGB::TestMsg*)tRxMsg);
      break;
   case RGB::MsgIdT::cRedRequestMsg:
      processRxMsg((RGB::RedRequestMsg*)tRxMsg);
      break;
   case RGB::MsgIdT::cGreenRequestMsg:
      processRxMsg((RGB::GreenRequestMsg*)tRxMsg);
      break;
   case RGB::MsgIdT::cBlueRequestMsg:
      processRxMsg((RGB::BlueRequestMsg*)tRxMsg);
      break;
   default:
      Prn::print(Prn::Show1, "ResponderThread::executeServerRxMsg ??? %d", tRxMsg->mMessageType);
      delete tRxMsg;
      break;
   }
   mRxCount++;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Message handler - TestMsg.

void ResponderThread::processRxMsg(RGB::TestMsg*  aRxMsg)
{
   aRxMsg->show(Prn::Show1);
   delete aRxMsg;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Rx message handler - RedRequestMsg.

void ResponderThread::processRxMsg(RGB::RedRequestMsg* aRxMsg)
{
   if (true)
   {
      RGB::RedResponseMsg* tTxMsg = new RGB::RedResponseMsg;
      tTxMsg->mCode1 = aRxMsg->mCode1;
      mSerialMsgThread->sendMsg(tTxMsg);
   }
   if (mShowCode == 3)
   {
      aRxMsg->show(Prn::Show1);
   }
   delete aRxMsg;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Rx message handler - GreenRequestMsg.

void ResponderThread::processRxMsg(RGB::GreenRequestMsg* aRxMsg)
{
   if (true)
   {
      RGB::GreenResponseMsg* tTxMsg = new RGB::GreenResponseMsg;
      tTxMsg->mCode1 = aRxMsg->mCode1;
      mSerialMsgThread->sendMsg(tTxMsg);
   }
   if (mShowCode == 3)
   {
      aRxMsg->show(Prn::Show1);
   }
   delete aRxMsg;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Rx message handler - BlueRequestMsg.

void ResponderThread::processRxMsg(RGB::BlueRequestMsg* aRxMsg)
{
   if (true)
   {
      RGB::BlueResponseMsg* tTxMsg = new RGB::BlueResponseMsg;
      tTxMsg->mCode1 = aRxMsg->mCode1;
      mSerialMsgThread->sendMsg(tTxMsg);
   }
   if (mShowCode == 3)
   {
      aRxMsg->show(Prn::Show1);
   }
   delete aRxMsg;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Send a message via mSerialMsgThread:

void ResponderThread::sendMsg(RGB::BaseMsg* aTxMsg)
{
   mSerialMsgThread->sendMsg(aTxMsg);
   mTxCount++;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Send a message via mSerialMsgThread:

void ResponderThread::sendTestMsg()
{
   RGB::TestMsg* tMsg = new RGB::TestMsg;
   tMsg->mCode1 = 201;

   mSerialMsgThread->sendMsg(tMsg);
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
}//namespace