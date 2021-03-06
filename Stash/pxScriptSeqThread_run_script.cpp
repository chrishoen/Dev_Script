/*==============================================================================
Description:
==============================================================================*/

//******************************************************************************
//******************************************************************************
//******************************************************************************

#include "stdafx.h"

#include "cmnShare.h"
#include "logLogFileThread.h"
#include "fcomGCodeCommThread.h"
#include "lcdGraphicsThread.h"

#include "pxPrinterParms.h"
#include "pxModel.h"
#include "cxFileManager.h"
#include "ccomControlThread.h"

#include "pxScriptSeqThread.h"

namespace PX
{

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Run script function. This is bound to the qcall. This runs the last 
// script1 file that was generated by the file manager. It is passed a 
// resume script flag. If the flag is false then the script is new and
// the state should be reset and the script file reader should start at
// the beginning of the file. if the flag is true then the script is being
// resumed from a script suspend and the state should be restored and
// the script reader should start where it left off when the suspend
// ocurred. True means run the script file. False means resume the script
// file.
// 
// This is used for running or resuming zip script files.

void ScriptSeqThread::executeRunScript1(bool aResumeFlag)
{
   // Initialize for run script.
   if (!aResumeFlag)
   {
      // Print and log.
      Prn::print(0, "");
      Prn::print(0, "running script1");
      Prn::print(Prn::Show1, "ScriptSeqThread::executeRunScript1 BEGIN");
      Log::TString* tString1 = new Log::TString;
      tString1->print("SCRIPT1 START//////////////////////////// %s",
         CX::gFileManager.mWorkGCodeFilePath.c_str());
      tString1->sendToLogFile();

      // Initialize variables.
      mLoopExitCode = 0;
      mSuspendRequestFlag = false;
      mSuspendExitFlag = false;

      // Open the script file.
      mReadCount = 0;
      if (!mReader.doOpenFile(CX::gFileManager.mWorkScriptFilePath1)) throw 991;

      // Set the state and send a message to the controller.
      Cmn::gShare.setStateSX1_Running();
      Cmn::gShare.initializeTime1();
      CCom::gControlThread->sendRunZipStateMsg();

      // Begin astop processing.
      Cmn::gShare.setAStopCode("AStopBegin");
   }
   // Initialize for resume script.
   else
   {
      // Print and log.
      Prn::print(0, "");
      Prn::print(0, "resuming script1");
      Prn::print(Prn::Show1, "ScriptSeqThread::executeRunScript1 BEGIN");
      Log::TString* tString1 = new Log::TString;
      tString1->print("SCRIPT1 RESUME/////////////////////////// %s",
         CX::gFileManager.mWorkGCodeFilePath.c_str());
      tString1->sendToLogFile();

      // Initialize variables.
      mLoopExitCode = 0;
      mSuspendRequestFlag = false;
      mSuspendExitFlag = false;
      Cmn::gShare.setStateSX1_Running();

      // Open the script file.
      if (!mReader.doOpenFile(CX::gFileManager.mWorkScriptFilePath1)) throw 991;

      // Read fast forward in the script file to the location at which
      // the script was suspended.
      for (int i = 0; i < mReadCount; i++)
      {
         // Read a command from the script file. Exit if end of file.
         if (!mReader.doRead()) break;
      }

      // Set the state and send a message to the controller.
      Cmn::gShare.setStateSX1_Running();
      CCom::gControlThread->sendRunZipStateMsg();
   }

   try
   {
      // Loop through the script file.
      while (true)
      {
         // Read a command from the script file. Exit if end of file.
         if (!mReader.doRead()) break;
         mReadCount++;

         // Test for a notification exception.
         mNotify.testException();

         // Delay.
         mNotify.waitForTimer(gPrinterParms.mScriptThrottle);

         // Execute the script file command.
         switch (mReader.mCmdCode)
         {
         case CX::cScriptCmd_Send:    doRunCmd_Send(1);   break;
         case CX::cScriptCmd_Slice:   doRunCmd_Slice();   break;
         case CX::cScriptCmd_Test:    doRunCmd_Test();    break;
         case CX::cScriptCmd_CMarker: doRunCmd_CMarker(); break;
         case CX::cScriptCmd_Info:    doRunCmd_Info();    break;
         }

         // Test the suspend exit flag.
         if (mSuspendExitFlag)
         {
            // Exit the loop.
            mLoopExitCode = cLoopExitSuspended;
            break;
         }
      }
   }
   catch(int aException)
   {
      mLoopExitCode = cLoopExitAborted;
      Prn::print(0, "EXCEPTION ScriptSeqThread::executeRunScript1 %d %s", aException, mNotify.mException);
   }

   // Send a blank screen request to the lcd graphics thread.
   // Do not wait for a completion notification.
   if (Lcd::gGraphicsThread) Lcd::gGraphicsThread->postDraw0(0);

   // Test the exit code.
   if (mLoopExitCode == cLoopExitNormal)
   {
      // Print and log.
      Prn::print(0, "script1 done");
      Prn::print(0, "");
      Log::TString* tString2 = new Log::TString("SCRIPT1 DONE");
      tString2->sendToLogFile();

      // Send a message to the controller.
      CCom::gControlThread->sendRunZipProgressMsg();

      // Set the state and send a message to the controller.
      Cmn::gShare.setStateSX1_Done();
      CCom::gControlThread->sendRunZipStateMsg();

      // Set the state and send a message to the controller.
      Cmn::gShare.setStateSX1_None();
      CCom::gControlThread->sendRunZipStateMsg();
   }
   else if (mLoopExitCode == cLoopExitSuspended)
   {
      // Print and log.
      Prn::print(0, "script1 suspended");
      Prn::print(0, "");
      Log::TString* tString2 = new Log::TString("SCRIPT1 SUSPENDED");
      tString2->sendToLogFile();

      // Set the state and send a message to the controller.
      Cmn::gShare.setStateSX1_Suspended();
      CCom::gControlThread->sendRunZipStateMsg();
   }
   else if (mLoopExitCode == cLoopExitAborted)
   {
      // Print and log.
      Prn::print(0, "script1 aborted");
      Prn::print(0, "");
      Log::TString* tString2 = new Log::TString("SCRIPT1 ABORTED");
      tString2->sendToLogFile();

      // Set the state and send a message to the controller.
      Cmn::gShare.setStateSX1_Aborted();
      CCom::gControlThread->sendRunZipStateMsg();
   }

   // End astop processing.
   Cmn::gShare.setAStopCode("AStopEnd");

   // Close the script file.
   mReader.doCloseFile();

   Prn::print(Prn::Show1, "ScriptSeqThread::executeRunScript1 END");
}

//******************************************************************************
//******************************************************************************
//******************************************************************************

void ScriptSeqThread::executeRunScript2()
{
   // Print and log.
   Prn::print(0, "");
   Prn::print(0, "running script2");
   Prn::print(Prn::Show1, "ScriptSeqThread::executeRunScript2 BEGIN");
   Log::TString* tString1 = new Log::TString;
   tString1->print("SCRIPT2 START//////////////////////////// %s",
      CX::gFileManager.mWorkGCodeFilePath.c_str());
   tString1->sendToLogFile();

   // Initialize variables.
   mLoopExitCode = 0;
   Cmn::gShare.setStateSX2_Running();
   Cmn::gShare.initializeTime2();

   // Open the script file.
   mReadCount = 0;
   if (!mReader.doOpenFile(CX::gFileManager.mWorkScriptFilePath2)) throw 991;

   // Set the state and send a message to the controller.
   Cmn::gShare.setStateSX2_Running();
   CCom::gControlThread->sendRunSnipStateMsg();

   try
   {
      // Loop through the script file.
      while (true)
      {
         // Read a command from the script file. Exit if end of file.
         if (!mReader.doRead()) break;
         mReadCount++;

         // Test for a notification exception.
         mNotify.testException();

         // Delay.
         mNotify.waitForTimer(gPrinterParms.mScriptThrottle);

         // Execute the script file command.
         switch (mReader.mCmdCode)
         {
         case CX::cScriptCmd_Send:    doRunCmd_Send(2);    break;
         case CX::cScriptCmd_Slice:   doRunCmd_Slice();    break;
         case CX::cScriptCmd_CMarker: doRunCmd_CMarker();  break;
         case CX::cScriptCmd_Info:    doRunCmd_Info();     break;
         }
      }
   }
   catch (int aException)
   {
      mLoopExitCode = cLoopExitAborted;
      Prn::print(0, "EXCEPTION ScriptSeqThread::executeRunScript2 %d %s", aException, mNotify.mException);
   }

   // Send a blank screen request to the lcd graphics thread.
   // Do not wait for a completion notification.
   if (Lcd::gGraphicsThread) Lcd::gGraphicsThread->postDraw0(0);

   // Test the exit code.
   if (mLoopExitCode == cLoopExitNormal)
   {
      // Print and log.
      Prn::print(0, "script2 done");
      Prn::print(0, "");
      Log::TString* tString2 = new Log::TString("SCRIPT2 DONE");
      tString2->sendToLogFile();

      // Send a message to the controller.
      CCom::gControlThread->sendRunSnipProgressMsg();

      // Set the state and send a message to the controller.
      Cmn::gShare.setStateSX2_Done();
      CCom::gControlThread->sendRunSnipStateMsg();

      // Set the state and send a message to the controller.
      Cmn::gShare.setStateSX2_None();
      CCom::gControlThread->sendRunZipStateMsg();
   }
   else if (mLoopExitCode == cLoopExitAborted)
   {
      // Print and log.
      Prn::print(0, "script2 aborted");
      Prn::print(0, "");
      Log::TString* tString2 = new Log::TString("SCRIPT2 ABORTED");
      tString2->sendToLogFile();

      // Set the state and send a message to the controller.
      Cmn::gShare.setStateSX2_Aborted();
      CCom::gControlThread->sendRunSnipStateMsg();
   }

   // Close the script file.
   mReader.doCloseFile();

   Prn::print(Prn::Show1, "ScriptSeqThread::executeRunScript2 END");
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Execute script command.

void ScriptSeqThread::doRunCmd_Info()
{
   Prn::print(Prn::Show1, "INFO  %s", mReader.mString);
   // Log the string.
   Log::TString* tTString = new  Log::TString(mReader.mString);
   tTString->sendToLogFile(Log::cLogAsInfo);

   // Send a message to the controller.
   CCom::gControlThread->sendViewStringMsg(mReader.mString);
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Execute script command.

void ScriptSeqThread::doRunCmd_Send(int aCode)
{
   // Guard.
   if (!FCom::gGCodeCommThread) return;

   // Set status.
   if (aCode == 1) Cmn::gShare.mGCodeTxCount1++;
   if (aCode == 2) Cmn::gShare.mGCodeTxCount2++;
   Cmn::gShare.setStateGX_Running();

   // Send a message to the controller.
   CCom::gControlThread->ControlThread::sendStateGXMsg(mReader.mString);

   // Set the thread notification mask.
   mNotify.setMaskOne("GCodeAck", cGCodeAckNotifyCode);

   // Modify the gcode string.
   gModel.doModify(mReader.mString);

   // Send the gcode command to the feynman. When The gcode thread receives
   // the corresponding acknowledgement response from the feynman it will
   // signal a completion notification.
   FCom::gGCodeCommThread->mSendGCodeWithNotifyCompletionQCall(new std::string(mReader.mString), mGCodeAckNotify);

   // Wait for the gcode acknowledgement notification.
   mNotify.wait(cGCodeAckTimeout);

   // Set status.
   Cmn::gShare.setStateGX_None();

   // Send a message to the controller.
   CCom::gControlThread->ControlThread::sendStateGXMsg();
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Execute script command.

void ScriptSeqThread::doRunCmd_Slice()
{
   // Guard.
   if (!Lcd::gGraphicsThread)
   {
      Cmn::gShare.mSliceCount++;
      if (gPrinterParms.mSliceCode == 1)
      {
         Cmn::gShare.setStateLX_Running();
         Ris::Threads::threadSleep(1000);
         Cmn::gShare.setStateLX_None();
      }
      // Send a message to the controller.
      CCom::gControlThread->sendSliceMsg(mReader.mString);
      return;
   }

   Prn::print(Prn::Show1, "SLICE %s", mReader.mString);
   // Log the string.
   Log::TString* tTString = new  Log::TString(mReader.mString);
   tTString->sendToLogFile(Log::cLogAsSlice);

   // Set status.
   Cmn::gShare.mSliceCount++;
   Cmn::gShare.setStateLX_Running();

   // Set the thread notification mask.
   mNotify.setMaskOne("Lcd", cLcdNotifyCode);

   // Send a png file load request to the lcd graphics thread.
   // It will send a completion notification.
   Lcd::gGraphicsThread->postDraw1(mReader.mString, &mLcdNotify);

   // Wait for the completion notification.
   mNotify.wait(cLcdTimeout);

   // Set status.
   Cmn::gShare.setStateLX_None();

   // Send a message to the controller.
   CCom::gControlThread->sendSliceMsg(mReader.mString);
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Execute script command.

void ScriptSeqThread::doRunCmd_PWM()
{
   Prn::print(Prn::Show1, "PWM %s", mReader.mString);
   // Log the string.
   Log::TString* tTString = new  Log::TString(mReader.mString);
   tTString->sendToLogFile(Log::cLogAsPWM);

   // Write the state value to the gpio pwm.
   int tValue = std::stoi(mReader.mString);

   // Sleep.
   Ris::Threads::threadSleep(200);
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Execute script command.

void ScriptSeqThread::doRunCmd_Test()
{
   Prn::print(Prn::Show1, "TEST  %s", mReader.mString);
   // Log the string.
   Log::TString* tTString = new  Log::TString(mReader.mString);
   tTString->sendToLogFile(Log::cLogAsTest);

   // If a suspend request is pending then set the suspend exit flag.
   // The script loop will then exit.
   if (mSuspendRequestFlag)
   {
      // Reset the flag.
      mSuspendRequestFlag = false;

      // Set the exit flag.
      mSuspendExitFlag = true;
   }
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Execute script command.

void ScriptSeqThread::doRunCmd_CMarker()
{
   Prn::print(Prn::Show1, "MARKER %s", mReader.mString);
   // Log the string.
   Log::TString* tTString = new  Log::TString(mReader.mString);
   tTString->sendToLogFile(Log::cLogAsCMarker);

   // Set the vstop code from the marker string.
   Cmn::gShare.setAStopCode(mReader.mString);

   // Send a message to the controller.
   CCom::gControlThread->sendMarkerMsg(mReader.mString);
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
}//namespace