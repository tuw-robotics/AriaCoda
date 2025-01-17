/*
Adept MobileRobots Robotics Interface for Applications (ARIA)
Copyright (C) 2004-2005 ActivMedia Robotics LLC
Copyright (C) 2006-2010 MobileRobots Inc.
Copyright (C) 2011-2015 Adept Technology, Inc.
Copyright (C) 2016-2018 Omron Adept Technologies, Inc.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 2 of the License, or
     (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


*/
#ifndef ARVCC4_H
#define ARVCC4_H

#include "Aria/ariaTypedefs.h"
#include "Aria/ArBasePacket.h"
#include "Aria/ArPTZ.h"
#include "Aria/ariaUtil.h"

// maximum number of bytes expected for a response from the camera
#define MAX_RESPONSE_BYTES 14

// the state timeout when using bidirectional communication
// This is big because it may have to wait for a power on or
// power off command to complete, which take ~4 seconds.
#define BIDIRECTIONAL_TIMEOUT 5000

// The number of ms to wait for a timeout for unidirectional communication.
// This is how long the usertask will wait before assuming that the camera
// has processed the last command.
#define UNIDIRECTIONAL_TIMEOUT 300

// how often to request position information from the camera if using
// bidirectional communication (in ms)
#define AUTO_UPDATE_TIME 2000

// accuracy of camera movements.  This sets how different the current 
// position and the desired position must be in order for a command to be
// sent to the camera.
#define TOLERANCE .1

/** @class ArVCC4
 *  Control the pan, tilt, and zoom mechanisms of the Canon VC-C4 and VC-C50i cameras.
 *
 *  An ArVCC4 object can be used to control the pan, tilt, zoom and some
 *  other aspects of the Canon VC-C4 camera.  Since the camera is 
 *  typically connected to the robot microcontroller's auxiliary serial
 *  port, and also uses ArRobot task cycle callbacks, a connected and
 *  running ArRobot object is required. 
 *
 *  Communication with the camera can operate in two modes or directions. 
 *  In unidirectional mode(COMM_UNIDIRECTIONAL),
 *  ArVCC4 simply sends commands to the camera, and waits for
 *  some time to allow the camera to process it. However, it will have
 *  no way of verifying that a command was successfully received by the 
 *  cameral. In bidirectional mode (COMM_BIDIRECTIONAL), ArVCC4 waits for a 
 *  response from the camera. Bidirectional mode requires that the CTS 
 *  line (pin 2 on the VISCA port) be connected. 
 *  When you create an ArVCC4 object, you can request a specific mode, 
 *  or you can specify COMM_UNKNOWN, and ArVCC4 will switch into
 *  bidirectional mode if it receives any responses from the camera.
 *
 *  Programmer's manuals for the VC-C45 and VC-C50i, detailing the
 *  communications protocol (including commands not implemented by ArVCC4),
 *  as well as user manuals containing specifications and hardware information,
 *  are available at <a href="http://robots.mobilerobots.com">http://robots.mobilerobots.com</a>. 
 *  ArVCC4 sends commands to the camera using the ArVCC4Packet class to construct the command, and
 *  using either the ArRobot pointer or an internal ArDeviceConnection,
 *  depending on whether the camera is connected to the robot 
 *  microcontroller's auxiliary serial port (the usual connection method for
 *  most robots) or a computer serial port.
 * 
 * @todo Implement setFocus() (send "Focus Position Assignment" command 0xB0 to camera)

\section VCC4CommandDetails Command-Response details:

This camera has a reponse mechanism, whereby each packet sent to the camera generates an answer within 300ms.  For the most part, the answer consists of a 6-byte packet which has an error-status within it.  Some commands generate longer packets.  Receiving the error status is helpful in that you know that the camera will or will not execute the command.  However, it doesn't tell you when the command is completed.

In order for the the responses to work, the CTS line on the camera must be high.  This is pin 2 on the VISCA port.  If your camera is not wired in such a fashion, then no answers will be sent to the computer, and the computer will not know whether or not the last packet was processed correctly.  Because of this, systems operating without the answer feature will need to run with delays between sending packets.  Otherwise, packets will be ignored, but you will have no way of knowing that.  To achieve this, there are two types of communication modes that this class will operate under - COMM_UNIDIRECTIONAL or COMM_BIDIRECTIONAL.  The default is COMM_UNKNOWN, in which it will use bidirectional communication if a response is received.

To handle the states and packet processing, this class runs as a user-task, different than the other pan/tilt devices.  Because of this, it must have a valid robot connection and a valid serial connection if using a computer serial port.  Note that the computer port must be set independently of this class.  The aux port can be selected via setAuxPort from the ArPTZ class.

\section VCC4UnitConversions Unit Conversions:

The camera's pan and tilt commands work on a number of units equal to (degrees / 0.1125).  The panTilt function always rounds the conversion closer to zero, so that a magnitude greater than the allowable range of movement is not sent to the camera.


\section VCC4C50iFeatures C50i features:

NEW - There is now limited support for the night-mode version of the C50i.  To enable night-mode support, pass the camera type in with the constructor.  Night-mode consists of two parts - a physical IR-cutoff filter, and IR LEDs.  The cutoff filter must be enabled first, then turn on the IR LEDs.

This camera has a digital zoom as well as the optical one.  There is an additional function for handling the digital.  There is also limited support for the auto-focus mechanism, which may need to be elaborated on for better night-vision.  In addition to the focus, there are also gain and backlight adjustments that can be made, but are not yet implemented in this class.
*/

/// Used by the ArVCC4 class
class ArVCC4Commands
{
public:
  enum Command  : uint8_t {
    DELIM = 0x00, ///<Delimeter character
    DEVICEID = 0x30, ///<Default device ID
    PANSLEW = 0x50, ///<Sets the pan slew
    TILTSLEW = 0x51, ///<Sets the tilt slew
    STOP = 0x53, ///<Stops current pan/tilt motion
    INIT = 0x58, ///<Initializes the camera
    SLEWREQ = 0x59, ///<Request pan/tilt min/max slew
    ANGLEREQ = 0x5c, ///<Request pan/tilt min/max angle
    PANTILT = 0x62, ///<Pan/tilt command
    SETRANGE = 0x64, ///<Pan/tilt min/max range assignment
    PANTILTREQ = 0x63, ///<Request pan/tilt position
    INFRARED = 0x76, ///<Controls operation of IR lighting
    PRODUCTNAME = 0x87, ///<Requests the product name
    LEDCONTROL = 0x8E, ///<Controls LED status
    CONTROL = 0x90, ///<Puts camera in Control mode
    POWER = 0xA0, ///<Turns on/off power
    AUTOFOCUS = 0xA1, ///<Controls auto-focusing functions
    ZOOMSTOP = 0xA2, ///<Stops zoom motion
    GAIN = 0xA5, ///<Sets gain adjustment on camera
    FOCUS = 0xB0, ///<Manual focus adjustment
    ZOOM = 0xB3, ///<Zooms camera lens
    ZOOMREQ = 0xB4, ///<Requests max zoom position
    IRCUTFILTER = 0xB5, ///<Controls the IR cut filter
    DIGITALZOOM = 0xB7, ///<Controls the digital zoom amount
    FOOTER = 0xEF, ///<Packet Footer
    RESPONSE = 0xFE, ///<Packet header for response
    HEADER = 0xFF ///<Packet Header
  };

};

/// Used by ArVCC4 to construct command packets
/** @internal
    There are only a few functioning ways to put things into this packet, you
    MUST use thse, if you use anything else your commands won't work.  You 
    must use only byteToBuf(), uByteToBuf(), byte4ToBuf(), and string and 
    data packing methods, no other ArBasePacket methods are implemented for VCC4.
*/
class ArVCC4Packet: public virtual ArBasePacket
{
public:
  /// Constructor
  ArVCC4Packet(uint16_t bufferSize = 30);
  //virtual ~ArVCC4Packet();

  virtual void byte4ToBuf(int32_t val) override;

  void byte2ToBuf(int32_t val) = delete; ///< previous versions overrode byte2ToBuf() but erroneously used a Byte4 argument type. Disable that; use byte4ToBuf() instead.


  virtual void finalizePacket() override;

private:
  // not implemented, make them private
  using ArBasePacket::uByte2ToBuf;
  using ArBasePacket::uByte4ToBuf;
  using ArBasePacket::uByte8ToBuf;
  using ArBasePacket::byte2ToBuf;
  using ArBasePacket::byte8ToBuf;
};

class ArVCC4 : public virtual ArPTZ
{
public:
  // the states for communication
  enum CommState
  {
    COMM_UNKNOWN,
    COMM_BIDIRECTIONAL,
    COMM_UNIDIRECTIONAL
  };

  enum CameraType
  {
    CAMERA_VCC4,
    CAMERA_C50I
  };

  /// Constructor
  AREXPORT explicit ArVCC4(ArRobot *robot, bool inverted = false, CommState commDirection = COMM_UNKNOWN, bool autoUpdate = true, bool disableLED = false, CameraType cameraType = CAMERA_VCC4);
  /// Destructor
  AREXPORT virtual ~ArVCC4();

  bool power(bool state)
  {
    myPowerStateDesired = state;
    return true;
  }
  bool getPower() { return myPowerState; }
  virtual bool init() override
  {
    myInitRequested = true;
    return true;
  }
  virtual void reset() override
  {
    ArPTZ::reset();
    init();
  }
  virtual const char *getTypeName() override { return "vcc4"; }

  /// Returns true if the camera has been initialized
  bool isInitted() { return myCameraIsInitted; }
  AREXPORT virtual void connectHandler() override;
  AREXPORT virtual bool packetHandler(ArBasePacket *packet) override;

protected:
  virtual bool pan_i(double deg) override { return panTilt_i(deg, myTiltDesired); }
  virtual bool panRel_i(double deg) override { return panTilt_i(myPanDesired + deg, myTiltDesired); }
  virtual bool tilt_i(double deg) override { return panTilt_i(myPanDesired, deg); }
  virtual bool tiltRel_i(double deg) override { return panTilt_i(myPanDesired, myTiltDesired + deg); }
  virtual bool panTiltRel_i(double pdeg, double tdeg) override { return panTilt_i(myPanDesired + pdeg, myTiltDesired + tdeg); }

public:
  /*
AREXPORT virtual double getMaxPosPan() const 
  { if (myInverted) return invert(MIN_PAN); else return MAX_PAN; }
AREXPORT virtual double getMaxNegPan() const 
  { if (myInverted) return invert(MAX_PAN); else return MIN_PAN; }
AREXPORT virtual double getMaxPosTilt() const 
  { if (myInverted) return invert(MIN_TILT); else return MAX_TILT; }
AREXPORT virtual double getMaxNegTilt() const
  { if (myInverted) return invert(MAX_TILT); else return MIN_TILT; }
*/

  /// Requests that a packet be sent to the camera to retrieve what
  /// the camera thinks are its pan/tilt positions. getPan() and getTilt()
  /// will then return this information instead of your last requested values.
  void getRealPanTilt() { myRealPanTiltRequested = true; }

  /// Requests that a packet be sent to the camera to retrieve what
  /// the camera thinks is its zoom position. getZoom()
  /// will then return this information instead of your last requested value.
  void getRealZoomPos() { myRealZoomRequested = true; }

  virtual bool canZoom() const override { return true; }

protected:
  AREXPORT virtual bool panTilt_i(double pdeg, double tdeg) override;

public:
  AREXPORT virtual bool zoom(int deg) override;
  /// adjust the digital zoom amount.  Has four states, takes 0-3 for:
  /// 1x, 2x, 4x, 8x
  AREXPORT bool digitalZoom(int deg);

  /// Adds an error callback to a list of callbacks to be called when there
  /// is a serious error in communicating - either the parameters were incorrect,
  /// the mode was incorrect, or there was an unknown error.
  AREXPORT void addErrorCB(ArFunctor *functor, ArListPos::Pos position);

  /// Remove an error callback from the callback list
  AREXPORT void remErrorCB(ArFunctor *functor);

  /// Halts all pan-tilt movement
  virtual bool haltPanTilt() override
  {
    myHaltPanTiltRequested = true;
    return true;
  }
  /// Halts zoom movement
  virtual bool haltZoom() override
  {
    myHaltZoomRequested = true;
    return true;
  }

  /// Sets the rate that the unit pans at
  virtual bool panSlew(double deg) override
  {
    myPanSlewDesired = deg;
    return true;
  }
  /// Sets the rate the unit tilts at
  virtual bool tiltSlew(double deg) override
  {
    myTiltSlewDesired = deg;
    return true;
  }
  bool canSetPanTiltSlew() { return true; }

private:
  /// Adds device ID and delimeter to myPacket
  void preparePacket();

protected:
  virtual double getPan_i() const override { return myPanDesired; }
  virtual double getTilt_i() const override { return myTiltDesired; }

public:
  virtual int getZoom() const override { return myZoomDesired; }
  double getDigitalZoom() const { return myDigitalZoomDesired; }

  virtual bool canGetRealPanTilt() const override { return true; }
  virtual bool canGetRealZoom() const override { return true; }
  virtual bool canSetFocus() const override { return false; }

  virtual bool setAutoFocus(bool af = true) override
  {
    if(af)
      myFocusModeDesired = 0;
    else
      myFocusModeDesired = 1;
    return true;
  }

  /// Set autofocus mode:
  /// @deprecated use setAutoFocus() instead
  bool autoFocus() { return setAutoFocus(true); }
  /// set manual focus mode
  /// @deprecated use setAutoFocus() instead
  bool manualFocus() { return setAutoFocus(false); }
  /// auto-focus on a near object
  bool focusNear()
  {
    myFocusModeDesired = 2;
    return true;
  }
  /// auto-focus on a far object
  bool focusFar()
  {
    myFocusModeDesired = 3;
    return true;
  }

  /// Gets the current pan slew
  double getPanSlew() { return myPanSlewDesired; }
  /// Gets the maximum pan slew
  double getMaxPanSlew() { return MAX_PAN_SLEW; }
  /// Gets the minimum pan slew
  double getMinPanSlew() { return MIN_PAN_SLEW; }

  /// Gets the current tilt slew
  double getTiltSlew() { return myTiltSlewDesired; }
  /// Gets the maximum tilt slew
  double getMaxTiltSlew() { return MAX_TILT_SLEW; }
  /// Gets the minimum tilt slew
  double getMinTiltSlew() { return MIN_TILT_SLEW; }

  AREXPORT virtual int getMaxZoom() const override;
  virtual int getMinZoom() const override { return MIN_ZOOM; }

  virtual bool canGetFOV() override { return true; }
  /// Gets the field of view at maximum zoom
  virtual double getFOVAtMaxZoom() override { return myFOVAtMaxZoom; }
  /// Gets the field of view at minimum zoom
  virtual double getFOVAtMinZoom() override { return myFOVAtMinZoom; }

  /// Returns true if the error callback list was called during the last cycle
  bool wasError() { return myWasError; }

  /// Toggle the state of the auto-update
  void enableAutoUpdate() { myAutoUpdate = true; }
  void disableAutoUpdate() { myAutoUpdate = false; }
  bool getAutoUpdate() { return myAutoUpdate; }

  /// Set the control mode for the status LED on the front of the camera
  /// 0 = auto-control, 1 = Green ON, 2 = All OFF, 3 = Red ON, 4 = Orange ON
  void setLEDControlMode(int controlMode) { myDesiredLEDControlMode = controlMode; }
  /// Turn on IR LEDs.  IR-filter must be in place for LEDs to turn on
  void enableIRLEDs() { myDesiredIRLEDsMode = true; }
  /// Turn off IR LEDs
  void disableIRLEDs() { myDesiredIRLEDsMode = false; }
  /// Returns true if the IR LEDs are on
  bool getIRLEDsEnabled() { return myIRLEDsEnabled; }
  /// Enable physical IR cutoff filter
  void enableIRFilterMode() { myDesiredIRFilterMode = true; }
  /// Disable IR cutoff filter.  This also turns off the LEDs, if they're on
  void disableIRFilterMode() { myDesiredIRFilterMode = false; }
  /// Returns true if the IR cutoff filter is in place
  bool getIRFilterModeEnabled() { return myIRFilterModeEnabled; }

protected:
  // preset limits on movements.  Based on empirical data
  enum Param
  {
    MAX_PAN = 98,       // 875 units is max pan assignment
    MIN_PAN = -98,      // -875 units is min pan assignment
    MAX_TILT = 88,      // 790 units is max tilt assignment
    MIN_TILT = -30,     // -267 units is min tilt assignment
    MAX_PAN_SLEW = 90,  // 800 positions per sec (PPS)
    MIN_PAN_SLEW = 1,   // 8 positions per sec (PPS)
    MAX_TILT_SLEW = 69, // 662 positions per sec (PPS)
    MIN_TILT_SLEW = 1,  // 8 positions per sec (PPS)
    MAX_ZOOM_OPTIC = 1960,
    MIN_ZOOM = 0
  };

  // the various error states that the camera can return
  enum Error : uint8_t
  {
    CAM_ERROR_NONE = 0x30,   ///<No error
    CAM_ERROR_BUSY = 0x31,   ///<Camera busy, will not execute the command
    CAM_ERROR_PARAM = 0x35,  ///<Illegal parameters to function call
    CAM_ERROR_MODE = 0x39,   ///<Not in host control mode
    CAM_ERROR_UNKNOWN = 0xFF ///<Unknown error condition.  Should never happen
  };

  // the states of the FSM
  enum State
  {
    UNINITIALIZED,
    STATE_UNKNOWN,
    INITIALIZING,
    SETTING_CONTROL_MODE,
    SETTING_INIT_TILT_RATE,
    SETTING_INIT_PAN_RATE,
    SETTING_INIT_RANGE,
    POWERING_ON,
    POWERING_OFF,
    POWERED_OFF,
    POWERED_ON,
    AWAITING_INITIAL_POWERON,
    AWAITING_INITIAL_INIT,
    AWAITING_ZOOM_RESPONSE,
    AWAITING_PAN_TILT_RESPONSE,
    AWAITING_STOP_PAN_TILT_RESPONSE,
    AWAITING_STOP_ZOOM_RESPONSE,
    AWAITING_PAN_SLEW_RESPONSE,
    AWAITING_TILT_SLEW_RESPONSE,
    AWAITING_POS_REQUEST,
    AWAITING_ZOOM_REQUEST,
    AWAITING_LED_CONTROL_RESPONSE,
    AWAITING_IRLEDS_RESPONSE,
    AWAITING_IRFILTER_RESPONSE,
    AWAITING_PRODUCTNAME_REQUEST,
    AWAITING_DIGITAL_ZOOM_RESPONSE,
    AWAITING_FOCUS_RESPONSE,
    STATE_DELAYED_SWITCH,
    STATE_ERROR
  };

  // flips the sign if needed
  //double invert(double before) const
  //  { if (myInverted) return -before; else return before; }
  // bool myInverted;

  // true if there was an error during the last cycle
  bool myWasError;

  // the camera name.  "C50i" for C50i, and "VC-C" for VC-C4
  std::string myProductName;

  ArRobot *myRobot;
  ArDeviceConnection *myConn;
  ArBasePacket *newPacket;
  ArVCC4Packet myPacket;

  // timers for watching for timeouts
  ArTime myStateTime;
  ArTime myPacketTime;
  ArTime myIdleTime;

  // gets set to true if using an aux port vs computer serial port
  bool myUsingAuxPort;

  // delay variable, if delaying before switching to the next state
  int myStateDelayTime;

  // what type of communication the camera is using
  CommState myCommType;

  // used to read data if the camera is attached directly to a computer
  virtual ArBasePacket *readPacket() override;

  // the functor to add as a usertask
  ArFunctorC<ArVCC4> myTaskCB;

  // the actual task to be added as a usertask
  void camTask();

  // true when a response has been received from the camera, but has
  // not yet been acted on by the state machine
  bool myResponseReceived;

  bool myWaitingOnStop;
  bool myWaitingOnPacket;

  // the state of the state machine
  State myState;
  State myPreviousState;
  State myNextState;

  // used to switch between states in the state machine
  void switchState(State state, int delayTime = 0);

  // the max time before a state times out, and the time for a packet response
  // to timeout.  The difference being that a packet reponse can be received
  // immediately, but it could say that the camera is busy, meaning the state
  // has not yet completed
  int myStateTimeout;
  int myPacketTimeout;

  // request a packet from the microcontroller of size num bytes.
  // most camera responses are 6 bytes, so just use the default
  void requestBytes(int num = 6);

  // the buffer to store the incoming packet data in
  unsigned char myPacketBuf[50];
  size_t myPacketBufLen;

  // how many bytes we're still expecting to receive from the controller
  int myBytesLeft;

  // these all send commands to the camera.
  bool sendPanTilt();
  bool sendZoom();
  bool sendPanSlew();
  bool sendTiltSlew();
  bool sendPower();
  bool sendHaltPanTilt();
  bool sendHaltZoom();
  bool sendRealPanTiltRequest();
  bool sendRealZoomRequest();
  bool sendDigitalZoom();
  bool sendFocus();

  // this is currently not used because it doesn't work right
  bool sendProductNameRequest();

  // the camera type is used to specify VC-C4 vs. C50i
  CameraType myCameraType;
  bool myRequestProductName;

  bool sendLEDControlMode();
  bool sendCameraNameRequest();
  int myDesiredLEDControlMode;

  bool sendIRFilterControl();
  bool sendIRLEDControl();
  bool myIRLEDsEnabled;
  bool myDesiredIRLEDsMode;
  bool myIRFilterModeEnabled;
  bool myDesiredIRFilterMode;

  // These should only be used by the state machine to initialize the
  // camera for the first time
  bool setDefaultRange();
  bool setControlMode();
  bool sendInit();

  // process the packet data for a camera response that has accurate
  // pan/tilt positional information in it, and the product name
  void processGetPanTiltResponse();
  void processGetZoomResponse();
  void processGetProductNameResponse();

  // true if autoupdating of camera's position should be used
  bool myAutoUpdate;

  // cycle for stepping through various autoupdate resquests from the camera
  int myAutoUpdateCycle;

  // returns true if there is no reponse to a packet within the timeout
  // or also if the state times out.  The argument will overrid the default
  // timeout periods
  bool timeout(int mSec = 0);

  // internal reperesenstation of pan, tilt, and zoom positions
  double myPan;
  double myTilt;
  int myZoom;
  int myDigitalZoom;
  int myFocusMode;

  // used to store the returned positional values when requesting the true
  // position from the camera
  double myPanResponse;
  double myTiltResponse;
  int myZoomResponse;

  // the returned product name
  char myProductNameResponse[4];

  // the positions that were last sent to the camera.  These are needed
  // because the desired positions can change between time a command is
  // sent and before it succeeds.
  double myPanSent;
  double myTiltSent;
  int myZoomSent;
  double myPanSlewSent;
  double myTiltSlewSent;

  // internal representation of pan and tilt slew
  double myPanSlew;
  double myTiltSlew;

  // where the user has requested the camera move to
  double myPanDesired;
  double myTiltDesired;
  int myZoomDesired;
  int myDigitalZoomDesired;
  int myFocusModeDesired;

  // the pan an tilt slew that the user requested
  double myPanSlewDesired;
  double myTiltSlewDesired;

  // internal mirror of camera power state, and whether it's be initted
  bool myPowerState;
  bool myCameraIsInitted;

  // whether the user wants the camera on or off, or initialized
  bool myPowerStateDesired;
  bool myInitRequested;

  // whether the user has requested to halt movement
  bool myHaltZoomRequested;
  bool myHaltPanTiltRequested;

  // whether the camera has been initialized since instance inception
  bool myCameraHasBeenInitted;

  // true if the user has requested to update the camera's postion
  // from the data returned from the camera
  bool myRealPanTiltRequested;
  bool myRealZoomRequested;

  // the error state from the last packet received
  unsigned int myError;

  // our FOV numbers (these should change if we use the digital zoom)
  double myFOVAtMaxZoom;
  double myFOVAtMinZoom;

  // run through the list or error callbacks
  void throwError();

  // the list of error callbacks to step through when a error occurs
  std::list<ArFunctor *> myErrorCBList;

#ifndef ARIA_WRAPPER
  /// Used by ArPTZConnector to create an ArVCC4 object based on robot parameters and program options.
  /// @since 2.7.6
  /// @internal
  static ArPTZ* create(size_t index, ArPTZParams params, ArArgumentParser *parser, ArRobot *robot);
  /// Used by ArPTZConnector to create an ArVCC4 object based on robot parameters and program options.
  /// @since 2.7.6
  /// @internal
  static ArPTZConnector::GlobalPTZCreateFunc ourCreateFunc;
#endif

public:
#ifndef ARIA_WRAPPER
  static void registerPTZType(); ///<@internal Called by Aria::init() toregister this class with ArPTZConnector for vcc4 and vcc50i PTZ types. @since 2.7.6
#endif
};

#endif // ARVCC4_H

