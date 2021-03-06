#pragma once

/*==============================================================================
Serial message prototype thread class.
==============================================================================*/

//******************************************************************************
//******************************************************************************
//******************************************************************************

#include "risThreadsQCallThread.h"
#include "risSerialMsgThread.h"

#include "rgbMsg.h"

//******************************************************************************
//******************************************************************************
//******************************************************************************

namespace Some
{

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Serial message prototype thread class. It processes messages that are
// communicated via a serial channel. The messages follow the byte content
// binary message scheme.
//
// This is a prototype serial thread class. It inherits from BaseQCallThread to
// obtain a call queue based thread functionality.
//
// The prototype thread creates a child serial message thread that establishes
// and manages a serial connection, receives messages and passes them to the
// parent via a qcall callback, and allows for the transmission of messages.
// the child thread also notifies the parent thread of serial connection
// establishment/disestablishment via a qcall callback.
// 
// The prototype thread is based on a call queue and it uses qcalls to
// interface to the child thread. When the child thread detects a session
// change it invokes the prototypes thread's mSessionQCall, which defers
// execution of its executeSession member function. Likewise, when the child
// thread receives a message it invokes the serial thread's mRxMsgQCall, which
// defers  execution of its executeRxMsg member function. 
//
// The child thread provides the execution context for actually managing
// session changes and receiving messages. The parent thread provides the
// execution context for processing the session changes and the received 
// messages.
//

class  RequesterThread : public Ris::Threads::BaseQCallThread
{
public:
   typedef Ris::Threads::BaseQCallThread BaseClass;

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Members.

   // Serial message thread, this manages serial message connections and
   // message transmission and reception.
   Ris::SerialMsgThread*  mSerialMsgThread;

   // Message monkey used by mSerialMsgThread.
   RGB::MsgMonkey* mMsgMonkey;

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Members.

   // True if the serial connection is valid.
   bool mConnectionFlag;

   // State variables.
   int mTPCode;
   int mRxCount;
   int mTxCount;
   int mShowCode;

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Methods.

   // Constructor.
   RequesterThread();
  ~RequesterThread();

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Methods. Thread base class overloads.

   // Thread init function. This is called by the base class immedidately 
   // after the thread starts running. It creates and launches the 
   // child SerialMsgThread.
   void threadInitFunction() override;

   // Thread exit function. This is called by the base class immedidately
   // before the thread is terminated. It shuts down the child SerialMsgThread.
   void threadExitFunction() override;

   // Thread shutdown function. This calls the base class shutdownThread
   // function to terminate the thread. This executes in the context of
   // the calling thread.
   void shutdownThread() override;

   // Execute periodically. This is called by the base class timer.
   void executeOnTimer(int aTimerCount) override;

   // Show thread info for this thread and for child threads.
   void showThreadInfo() override;

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Methods. qcalls.

   // qcall registered to the mSerialMsgThread child thread. It is invoked
   // when a session is established or disestablished (when the serial port
   // is opened or it is closed because of an error or a disconnection). 
   Ris::SerialMsgThread::SessionQCall mSessionQCall;

   // Maintain session state variables. This is bound to the qcall.
   void executeSession(bool aConnected);

   // qcall registered to the mSerialMsgThread child thread. It is invoked by
   // the child thread when a message is received.
   Ris::SerialMsgThread::RxMsgQCall mRxMsgQCall;

   // Based on the receive message type, call one of the specific receive
   // message handlers. This is bound to the qcall.
   void executeRxMsg(Ris::ByteContent* aMsg);

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Methods. qcalls.

   // Abort qcall. It is invoked by the control thread to execute a
   // an abort in the context of this thread.
   Ris::Threads::QCall0 mAbortQCall;

   // Abort function. This is bound to the qcall. It aborts the serial port.
   void executeAbort();

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Methods.

   // Receive message handlers. There is one for each message that can 
   // be received.
   void processRxMsg(RGB::TestMsg*  aMsg);
   void processRxMsg(RGB::RedResponseMsg* aMsg);
   void processRxMsg(RGB::GreenResponseMsg* aMsg);
   void processRxMsg(RGB::BlueResponseMsg* aMsg);

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Methods.

   // Send a message via mSerialMsgThread:
   void sendMsg (RGB::BaseMsg* aTxMsg);
   void sendTestMsg();
};

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Global singular instance.

#ifdef _SOMEREQUESTERTHREAD_CPP_
         RequesterThread* gRequesterThread;
#else
extern   RequesterThread* gRequesterThread;
#endif

//******************************************************************************
//******************************************************************************
//******************************************************************************
}//namespace
