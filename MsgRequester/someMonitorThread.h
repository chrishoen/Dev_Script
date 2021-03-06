#pragma once

/*==============================================================================
Program monitor timer thread.
==============================================================================*/

//******************************************************************************
//******************************************************************************
//******************************************************************************

#include "risThreadsTimerThread.h"
#include "risMonitor.h"

namespace Some
{
//******************************************************************************
//******************************************************************************
//******************************************************************************
// This is a timer thread that monitors program status and prints it.

class MonitorThread : public Ris::Threads::BaseTimerThread
{
public:
   typedef Ris::Threads::BaseTimerThread BaseClass;

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Members.

   // If true then process status.
   bool mTPFlag;

   // Select show mode.
   int mShowCode;

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Members. Monitor variables.

   // Message counts.
   Ris::Monitor<int> mMon_TxMsgCount;
   Ris::Monitor<long long> mMon_TxByteCount;
   Ris::Monitor<int> mMon_TxRedRequestMsgCount;
   Ris::Monitor<int> mMon_TxGreenRequestMsgCount;
   Ris::Monitor<int> mMon_TxBlueRequestMsgCount;

   Ris::Monitor<int> mMon_RxMsgCount;
   Ris::Monitor<long long> mMon_RxByteCount;
   Ris::Monitor<int> mMon_RxRedResponseMsgCount;
   Ris::Monitor<int> mMon_RxGreenResponseMsgCount;
   Ris::Monitor<int> mMon_RxBlueResponseMsgCount;

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Methods.

   // Constructor.
   MonitorThread();

   //***************************************************************************
   //***************************************************************************
   //***************************************************************************
   // Methods. Thread base class overloads.

   // Update status variables.
   void update();

   // Execute periodically. This is called by the base class timer. It 
   // updates monitor variables and shows them as program status.
   void executeOnTimer(int aTimeCount) override;
};

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Global instance.

#ifdef _SOMEMONITORTHREAD_CPP_
           MonitorThread* gMonitorThread;
#else
   extern  MonitorThread* gMonitorThread;
#endif

//******************************************************************************
//******************************************************************************
//******************************************************************************
}//namespace


