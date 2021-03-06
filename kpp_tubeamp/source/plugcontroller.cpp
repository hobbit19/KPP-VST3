//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : plugcontroller.cpp
// Created by  : Steinberg, 01/2018
// Description : HelloWorld Example for VST 3
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2019, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this
//     software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "../include/plugcontroller.h"
#include "../include/plugids.h"
#include "pluginterfaces/base/ustring.h"

#include "base/source/fstreamer.h"
#include "base/source/fstring.h"
#include "pluginterfaces/base/ibstream.h"

#include "../include/pluguimessagecontroller.h"

using namespace VSTGUI;

namespace Steinberg {
namespace Vst {
  class ToneParameter : public Parameter
  {
  public:
    ToneParameter (int32 flags, int32 id, const char *label);

    void toString (ParamValue normValue, String128 string) const SMTG_OVERRIDE;
    bool fromString (const TChar* string, ParamValue& normValue) const SMTG_OVERRIDE;
  };

  ToneParameter::ToneParameter (int32 flags, int32 id, const char *label)
  {
    UString (info.title, USTRINGSIZE (info.title)).assign (USTRING (label));
    UString (info.units, USTRINGSIZE (info.units)).assign (USTRING ("dB"));

    info.flags = flags;
    info.id = id;
    info.stepCount = 0;
    info.defaultNormalizedValue = 0.5f;
    info.unitId = kRootUnitId;

    setNormalized (1.f);
  }

  void ToneParameter::toString (ParamValue normValue, String128 string) const
  {
    char text[32];
    sprintf (text, "%.2f", (normValue * 2.0 - 1.0) * 10.0);

    Steinberg::UString (string, 128).fromAscii (text);
  }

  bool ToneParameter::fromString (const TChar* string, ParamValue& normValue) const
  {
    String wrapper ((TChar*)string); // don't know buffer size here!
    double tmp = 0.0;
    if (wrapper.scanFloat (tmp))
    {
      normValue = (tmp + 10.0) / 20.0;
      return true;
    }
    return false;
  }

  class GainParameter : public Parameter
  {
  public:
    GainParameter (int32 flags, int32 id, const char * label);

    void toString (ParamValue normValue, String128 string) const SMTG_OVERRIDE;
    bool fromString (const TChar* string, ParamValue& normValue) const SMTG_OVERRIDE;
  };

  GainParameter::GainParameter (int32 flags, int32 id, const char * label)
  {
    UString (info.title, USTRINGSIZE (info.title)).assign (USTRING (label));
    UString (info.units, USTRINGSIZE (info.units)).assign (USTRING ("dB"));

    info.flags = flags;
    info.id = id;
    info.stepCount = 0;
    info.defaultNormalizedValue = 0.5f;
    info.unitId = kRootUnitId;

    setNormalized (1.f);
  }

  void GainParameter::toString (ParamValue normValue, String128 string) const
  {
    char text[32];
    sprintf (text, "%.2f", normValue * 100.0);

    Steinberg::UString (string, 128).fromAscii (text);
  }

  bool GainParameter::fromString (const Vst::TChar* string, Vst::ParamValue& normValue) const
  {
    String wrapper ((Vst::TChar*)string); // don't know buffer size here!
    double tmp = 0.0;
    if (wrapper.scanFloat (tmp))
    {
      normValue = tmp / 100.0;
      return true;
    }
    return false;
  }

  tresult PLUGIN_API PlugController::initialize (FUnknown* context)
  {
    tresult result = EditControllerEx1::initialize (context);
    if (result == kResultTrue)
    {
      parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
                               ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass,
                               kBypassId);

      GainParameter* driveParam = new GainParameter(ParameterInfo::kCanAutomate, kDriveId, "Drive");
      parameters.addParameter (driveParam);

      ToneParameter* bassParam = new ToneParameter (ParameterInfo::kCanAutomate, kBassId, "Bass");
      parameters.addParameter (bassParam);

      ToneParameter* middleParam = new ToneParameter (ParameterInfo::kCanAutomate, kMiddleId, "Middle");
      parameters.addParameter (middleParam);

      ToneParameter* trebleParam = new ToneParameter (ParameterInfo::kCanAutomate, kTrebleId, "Treble");
      parameters.addParameter (trebleParam);

      GainParameter* volumeParam = new GainParameter (ParameterInfo::kCanAutomate, kVolumeId, "Volume");
      parameters.addParameter (volumeParam);

      parameters.addParameter (STR16 ("Level"), NULL, 0, 1.0,
                               ParameterInfo::kCanAutomate, kLevelId, 0,
                               STR16 ("Level"));

      parameters.addParameter (STR16 ("Cabinet"), NULL, 0, 1.0,
                               ParameterInfo::kCanAutomate, kCabinetId, 0,
                               STR16 ("Cabinet"));
    }
    return kResultTrue;
  }

  IPlugView* PLUGIN_API PlugController::createView (const char* name)
  {
    if (name && strcmp (name, "editor") == 0)
    {
      view = new VST3Editor (this, "view", "plug.uidesc");
      return view;
    }
    return nullptr;
  }

  IController* PlugController::createSubController (UTF8StringPtr name,
                                                     const IUIDescription* /*description*/,
                                                     VST3Editor* /*editor*/)
  {
    if (UTF8StringView (name) == "MessageController")
    {
      auto* controller = new UIMessageController (this);
      controller->mainView = view;
      addUIMessageController (controller);
      return controller;
    }
    return nullptr;
  }

  tresult PLUGIN_API PlugController::setComponentState (IBStream* state)
  {
    if (!state)
      return kResultFalse;

    IBStreamer streamer (state, kLittleEndian);

    float savedDrive = 0.f;
    if (streamer.readFloat (savedDrive) == false)
      return kResultFalse;
    setParamNormalized (kDriveId, savedDrive);

    float savedBass = 0.f;
    if (streamer.readFloat (savedBass) == false)
      return kResultFalse;
    setParamNormalized (kBassId, savedBass);

    float savedMiddle = 0.f;
    if (streamer.readFloat (savedMiddle) == false)
      return kResultFalse;
    setParamNormalized (kMiddleId, savedMiddle);

    float savedTreble = 0.f;
    if (streamer.readFloat (savedTreble) == false)
      return kResultFalse;
    setParamNormalized (kTrebleId, savedTreble);

    float savedVolume = 0.f;
    if (streamer.readFloat (savedVolume) == false)
      return kResultFalse;
    setParamNormalized (kVolumeId, savedVolume);

    float savedLevel = 0.f;
    if (streamer.readFloat(savedLevel) == false)
      return kResultFalse;
    setParamNormalized(kLevelId, savedLevel);

    float savedCabinet = 0.f;
    if (streamer.readFloat (savedCabinet) == false)
      return kResultFalse;
    setParamNormalized (kCabinetId, savedCabinet);

    int32 bypassState;
    if (streamer.readInt32 (bypassState) == false)
      return kResultFalse;
    setParamNormalized (kBypassId, bypassState ? 1 : 0);

    return kResultOk;
  }

  tresult PLUGIN_API PlugController::setState(IBStream* state)
  {
    IBStreamer streamer(state, kLittleEndian);
    profilePath = streamer.readStr8();
    return kResultOk;
  }
  tresult PLUGIN_API PlugController::getState(IBStream* state)
  {
    IBStreamer streamer(state, kLittleEndian);
    streamer.writeStr8(profilePath.c_str());

    return kResultOk;
  }

  tresult PLUGIN_API PlugController::setParamNormalized (ParamID tag, ParamValue value)
  {
    tresult result = EditControllerEx1::setParamNormalized (tag, value);
    return result;
  }

  tresult PLUGIN_API PlugController::getParamStringByValue (ParamID tag, ParamValue valueNormalized,
                                                            String128 string)
  {
    return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
  }

  tresult PLUGIN_API PlugController::getParamValueByString (ParamID tag, TChar* string,
                                                            ParamValue& valueNormalized)
  {
    return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
  }

  void PlugController::addUIMessageController (UIMessageController* controller)
  {
    uiMessageControllers.push_back (controller);
  }

  void PlugController::removeUIMessageController (UIMessageController* controller)
  {
    UIMessageControllerList::const_iterator it =
    std::find (uiMessageControllers.begin (), uiMessageControllers.end (), controller);
    if (it != uiMessageControllers.end ())
      uiMessageControllers.erase (it);
  }

} // Vst
} // Steinberg
