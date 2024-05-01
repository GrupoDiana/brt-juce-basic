/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             brt-juce-basic.h
 version:          1.0.0
 vendor:           JUCE
 website:          http://diana.uma.es
 description:      Plays audio files.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2022

 type:             Component
 mainClass:        MainContentComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

#include <BRTLibrary.h>

//==============================================================================
constexpr int BLOCK_SIZE = 512;    // Block size in samples
constexpr int HRTFRESAMPLINGSTEP = 15;
constexpr float SOURCE1_INITIAL_AZIMUTH = 3.141592653589793 / 2.0; // pi/2
constexpr float SOURCE1_INITIAL_ELEVATION = 0.f;
constexpr float SOURCE1_INITIAL_DISTANCE = 1;// 0.1f; // 10 cm.

//==============================================================================
class MainContentComponent   : public juce::AudioAppComponent,
                               public juce::ChangeListener,
                               public juce::Slider::Listener,
                               public juce::Button::Listener
{
public:
    //==========================================================================
    MainContentComponent()
        : state (Stopped)
    {
        // Add a button to open a SOFA file
        addAndMakeVisible(&openSOFAButton);
        openSOFAButton.setButtonText("Open SOFA file...");
		openSOFAButton.onClick = [this] { openSOFAButtonClicked(); };

        addAndMakeVisible (&openWavButton);
        openWavButton.setButtonText ("Open mono wav file...");
        openWavButton.onClick = [this] { openWavButtonClicked(); };

        addAndMakeVisible (&playButton);
        playButton.setButtonText ("Play");
        playButton.onClick = [this] { playButtonClicked(); };
        playButton.setColour (juce::TextButton::buttonColourId, juce::Colours::green);
        playButton.setEnabled (false);

        addAndMakeVisible (&stopButton);
        stopButton.setButtonText ("Stop");
        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setColour (juce::TextButton::buttonColourId, juce::Colours::red);
        stopButton.setEnabled (false);

        // Add a dial to control the azimuth of the source
        addAndMakeVisible(&sourceAzimuthDial);
        sourceAzimuthDial.setRange(- juce::MathConstants<float>::pi / 2, juce::MathConstants<float>::pi / 2, 0.01);
        sourceAzimuthDial.setValue(SOURCE1_INITIAL_AZIMUTH);
        sourceAzimuthDial.setTextValueSuffix(" rad");
        sourceAzimuthDial.addListener(this);
        sourceAzimuthDial.setEnabled(false);
        
        addAndMakeVisible(&sourceAzimuthLabel);
        sourceAzimuthLabel.setText("Azimuth", juce::dontSendNotification);
		sourceAzimuthLabel.attachToComponent(&sourceAzimuthDial, true);
        sourceAzimuthLabel.setEnabled(false);

        addAndMakeVisible(&sourceElevationDial);
        sourceElevationDial.setRange(- juce::MathConstants<float>::pi / 4, juce::MathConstants<float>::pi / 4, 0.01);
        sourceElevationDial.setValue(SOURCE1_INITIAL_ELEVATION);
        sourceElevationDial.setTextValueSuffix(" rad");
        sourceElevationDial.addListener(this);
		sourceElevationDial.setEnabled(false);

        addAndMakeVisible(&sourceElevationLabel);
        sourceElevationLabel.setText("Elevation", juce::dontSendNotification);
        sourceElevationLabel.attachToComponent(&sourceElevationDial, true);
        sourceElevationLabel.setEnabled(false);

        addAndMakeVisible(&sourceDistanceDial);
        sourceDistanceDial.setRange(0.1, 10, 0.01);
        sourceDistanceDial.setValue(SOURCE1_INITIAL_DISTANCE);
        sourceDistanceDial.setTextValueSuffix(" m");
		sourceDistanceDial.addListener(this);
        sourceDistanceDial.setEnabled(false);

        addAndMakeVisible(&sourceDistanceLabel);
        sourceDistanceLabel.setText("Distance", juce::dontSendNotification);
        sourceDistanceLabel.attachToComponent(&sourceDistanceDial, true);
        sourceDistanceLabel.setEnabled(false);
        
        formatManager.registerBasicFormats();       // [1]
        transportSource.addChangeListener (this);   // [2]

        setAudioChannels (0, 2);
        
        if (auto* device = deviceManager.getCurrentAudioDevice())
        {
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            deviceManager.getAudioDeviceSetup(setup);
            
            // Show the sample rate in the GUI
            addAndMakeVisible(&sampleRateLabel);
            sampleRateLabel.setText("Sample Rate: " + std::to_string((int)setup.sampleRate) + " Hz", juce::dontSendNotification);
            sourceDistanceLabel.attachToComponent(&sourceDistanceDial, true);
            
            setup.bufferSize = BLOCK_SIZE; // Set the buffer size
            deviceManager.setAudioDeviceSetup(setup, true);
            setupBRT(setup.sampleRate, setup.bufferSize);
        }
        else {
			juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "No audio device found", "OK");
		}
        setSize (400, 300);

    }

	//==========================================================================
    ~MainContentComponent() override
    {
        shutdownAudio();
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
        globalParameters.SetSampleRate(sampleRate);
        globalParameters.SetBufferSize(samplesPerBlockExpected);
        // initialise parameters needed for changing HRTFs
        hrtfState = NotToBeChanged;
        selectedHRTFidx = -1;
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        // If we still haven't loaded a file, simply clear the buffer
        if (readerSource.get() == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        // Check if different HRTF was selected and change accordingly
        if (hrtfState == ToBeChanged){
            listener->SetHRTF(HRTF_list[selectedHRTFidx]);
            hrtfState = NotToBeChanged;
        }

        // Obtener las muestras de audio del transportSource
        juce::AudioBuffer<float> audioInputBuffer(1, bufferToFill.numSamples); // Buffer mono para las muestras de audio
        juce::AudioSourceChannelInfo audioBufferToFill(&audioInputBuffer, 0, bufferToFill.numSamples);
        transportSource.getNextAudioBlock(audioBufferToFill);

        // Prepare the input buffer for source 1
        CMonoBuffer<float> source1Buffer(bufferToFill.numSamples);
        for (int i = 0; i < bufferToFill.numSamples; i++) {
			source1Buffer[i] = audioInputBuffer.getSample(0, i);
		}
		// Pass the input buffer to the BRT Library source
		source1BRT->SetBuffer(source1Buffer);

       // Binaural processing
        brtManager.ProcessAll(); // Process all sources

        //// Get output stereo buffer
        Common::CEarPair<CMonoBuffer<float>> outputBuffer;
        listener->GetBuffers(outputBuffer.left, outputBuffer.right);
  

        // Left output buffer is expected to be in channel 0
        float* const leftBuffer = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
        float* const rightBuffer = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);
        for (int i = 0; i < bufferToFill.numSamples; i++) {
            leftBuffer[i] = outputBuffer.left[i];
            rightBuffer[i] = outputBuffer.right[i];
           
            // For test purposes just copy the sambples from source1Buffer to the output buffers
            // leftBuffer[i] = source1Buffer[i];
            // rightBuffer[i] = source1Buffer[i];  
        }	

    } 

    void releaseResources() override
    {
        transportSource.releaseResources();
    }

    void resized() override
    {
        openWavButton.setBounds(10, 10, getWidth() - 20, 20);
        openSOFAButton.setBounds (10, 40, getWidth() - 20, 20);
        playButton.setBounds (10, 70, getWidth() - 20, 20);
        stopButton.setBounds (10, 100, getWidth() - 20, 20);
        auto sliderLeft = 50;

        // Position nicely all sliders
        sourceAzimuthDial.setBounds(sliderLeft, 130, getWidth() - sliderLeft - 10, 20);
        sourceElevationDial.setBounds(sliderLeft, 160, getWidth() - sliderLeft - 10, 20);
        sourceDistanceDial.setBounds(sliderLeft, 190, getWidth() - sliderLeft - 10, 20);
        sampleRateLabel.setBounds(getWidth()-160, 220, getWidth()-20, 20);
        // Position the SOFA buttons at the bottom of the component
        int y = getHeight() - 30;
        for (auto* button : sofaFileButtons)
        {
            button->setBounds(10, y, getWidth() - 20, 20);
            y -= 30;
        }
        
        
    }

    void changeListenerCallback (juce::ChangeBroadcaster* source) override
    {
        if (source == &transportSource)
        {
            if (transportSource.isPlaying())
                changeState (Playing);
            else
                changeState (Stopped);
        }
    }

    void sliderValueChanged(juce::Slider* slider) override
	{
        // Make sure the slider is enabled before accessing BRT objects
        if (!slider->isEnabled())
        {
            return;
        }

		if (slider == &sourceAzimuthDial) {
			sourceAzimuth = sourceAzimuthDial.getValue();
			Common::CTransform sourceTransform = source1BRT->GetCurrentSourceTransform();
			Common::CVector3 listenerPosition = listener->GetListenerTransform().GetPosition();
			Common::CVector3 sourcePosition = Common::CVector3(sourceDistance * std::cos(sourceAzimuth) * std::cos(sourceElevation) + listenerPosition.x,
															   sourceDistance * std::sin(sourceAzimuth) * std::cos(sourceElevation) + listenerPosition.y,
															   sourceDistance * std::sin(sourceElevation) + listenerPosition.z);
			sourceTransform.SetPosition(sourcePosition);
			source1BRT->SetSourceTransform(sourceTransform);
		}
		else if (slider == &sourceElevationDial) {
			sourceElevation = sourceElevationDial.getValue();
			Common::CTransform sourceTransform = source1BRT->GetCurrentSourceTransform();
			Common::CVector3 listenerPosition = listener->GetListenerTransform().GetPosition();
			Common::CVector3 sourcePosition = Common::CVector3(sourceDistance * std::cos(sourceAzimuth) * std::cos(sourceElevation) + listenerPosition.x,
															   sourceDistance * std::sin(sourceAzimuth) * std::cos(sourceElevation) + listenerPosition.y,
															   sourceDistance * std::sin(sourceElevation) + listenerPosition.z);
			sourceTransform.SetPosition(sourcePosition);
			source1BRT->SetSourceTransform(sourceTransform);
		}
		else if (slider == &sourceDistanceDial) {
			sourceDistance = sourceDistanceDial.getValue();
			Common::CTransform sourceTransform = source1BRT->GetCurrentSourceTransform();
			Common::CVector3 listenerPosition = listener->GetListenerTransform().GetPosition();
			Common::CVector3 sourcePosition = Common::CVector3(sourceDistance * std::cos(sourceAzimuth) * std::cos(sourceElevation) + listenerPosition.x,
															   sourceDistance * std::sin(sourceAzimuth) * std::cos(sourceElevation) + listenerPosition.y,
															   sourceDistance * std::sin(sourceElevation) + listenerPosition.z);
			sourceTransform.SetPosition(sourcePosition);
			source1BRT->SetSourceTransform(sourceTransform);
		}
	}

    void buttonClicked(juce::Button* button) override
	{
        if (button->getToggleState())
        {
            // Find the button that was clicked in the list of SOFA file buttons
            for (int i = 0; i < sofaFileButtons.size(); ++i)
			{
				if (button == sofaFileButtons[i])
				{
					// Check if the button is not the same as previously selected
                    if (i != selectedHRTFidx)
                    {
                        //Set the listener HRTF to the selected SOFA file
                        selectedHRTFidx = i;
                        hrtfState = ToBeChanged;
                    }
					break;
				}
			}

        }
	}

private:
    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Stopping
    };
    // States used when HRTF is being changed
    enum HRTFState
    {
        ToBeChanged,
        NotToBeChanged
    };

    void changeState (TransportState newState)
    {
        if (state != newState)
        {
            state = newState;

            switch (state)
            {
                case Stopped:                           // [3]
                    stopButton.setEnabled (false);
                    playButton.setEnabled (true);
                    transportSource.setPosition (0.0);
                    break;

                case Starting:                          // [4]
                    playButton.setEnabled (false);
                    transportSource.start();
                    break;

                case Playing:                           // [5]
                    stopButton.setEnabled (true);
                    break;

                case Stopping:                          // [6]
                    transportSource.stop();
                    break;
            }
        }
    }

    //==========================================================================
    /// Setup BRT Library
    void setupBRT(int sampleRate, int bufferSize) {

        // Global parameters
        globalParameters.SetSampleRate(sampleRate);
        globalParameters.SetBufferSize(bufferSize);

        // Listener creation
        brtManager.BeginSetup();
        listener = brtManager.CreateListener<BRTListenerModel::CListenerHRTFbasedModel>("listener1");
        brtManager.EndSetup();

        // Place the listener in (0,0,0)
        Common::CTransform listenerPosition = Common::CTransform();
        listenerPosition.SetPosition(Common::CVector3(0, 0, 0));
        listener->SetListenerTransform(listenerPosition);
    }

    //==========================================================================
    /// Load a SOFA file and add it to the HRTF list
    bool LoadSOFAFile(const juce::File& file) {
        bool result = false; 
        std::shared_ptr<BRTServices::CHRTF> hrtf = std::make_shared<BRTServices::CHRTF>();

        // Try to get sample rate in SOFA
        int sampleRateInSOFAFile = sofaReader.GetSampleRateFromSofa(file.getFullPathName().toStdString());
        if (sampleRateInSOFAFile == -1) {
			juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "The SOFA file does not contain a valid sample rate", "OK");
			result = false;
		}
        else {
            // Make sure sample rate is same as selected in app. 
            if (sampleRateInSOFAFile != globalParameters.GetSampleRate()) {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "The SOFA file sample rate does not match the selected sample rate", "OK");
                result = false;
            }
			else {
				// Load SOFA file
                result = sofaReader.ReadHRTFFromSofa(file.getFullPathName().toStdString(), hrtf, HRTFRESAMPLINGSTEP, "NearestPoint");
                if (result) {
					HRTF_list.push_back(hrtf);
				}
				else {
					juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "Error loading SOFA file", "OK");
				}
			}
        }
        return result;  
    }

    //==========================================================================
    // Load a source in the BRT Library
    void LoadSource(const String& name, float azimuth, float elevation, float distance) {
		// Create a source
		brtManager.BeginSetup();
		source1BRT = brtManager.CreateSoundSource<BRTSourceModel::CSourceSimpleModel>(name.toStdString());
        listener->ConnectSoundSource(source1BRT);
		brtManager.EndSetup();

		// Set the source position
		Common::CVector3 listenerPosition = listener->GetListenerTransform().GetPosition();
        Common::CTransform sourceTransform;
        Common::CVector3 sourcePosition = Common::CVector3(distance * std::cos(azimuth) * std::cos(elevation) + listenerPosition.x,
														   distance * std::sin(azimuth) * std::cos(elevation) + listenerPosition.y,
														   distance * std::sin(elevation) + listenerPosition.z);
        sourceTransform.SetPosition(sourcePosition);
		source1BRT->SetSourceTransform(sourceTransform);
	}

    // Open a SOFA file using a file chooser
     void openSOFAButtonClicked()
	{
		chooser = std::make_unique<juce::FileChooser> ("Select a SOFA file to load...",
													   juce::File{},
													   "*.sofa");                     // Filter for SOFA files
		auto chooserFlags = juce::FileBrowserComponent::openMode
						  | juce::FileBrowserComponent::canSelectFiles;

		chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)     // Launch the file chooser
		{
			auto file = fc.getResult();

			if (file != juce::File{})                                                
			{
				// Load the SOFA file
				if (LoadSOFAFile(file)) {

					juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Success", "SOFA file loaded successfully", "OK");
					sourceAzimuthDial.setEnabled(true);
					sourceElevationDial.setEnabled(true);
					sourceDistanceDial.setEnabled(true);

                    // Create a new ToggleButton for the new SOFA file
                    ToggleButton* sofaFileButton = new ToggleButton(file.getFileNameWithoutExtension());
                    sofaFileButton->setRadioGroupId(1);
                    sofaFileButtons.add(sofaFileButton);
                    addAndMakeVisible(sofaFileButton);
                    sofaFileButton->addListener(this);
                    sofaFileButton->setToggleState(true, juce::NotificationType::dontSendNotification);

                    // Call resized() to update the layout
                    resized();

                    // Set the listener HRTF to the last loaded HRTF
                    selectedHRTFidx = (int) HRTF_list.size() - 1;
                    hrtfState = ToBeChanged; // The HRTF will be changed in the next buffer
                    
				}
			}
		});
	}

    // Open a mono wav file using a file chooser
    void openWavButtonClicked()
    {
        chooser = std::make_unique<juce::FileChooser> ("Select a Wave file to play...",
                                                       juce::File{},
                                                       "*.wav");                     // [7]
        auto chooserFlags = juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)     // [8]
        {
            auto file = fc.getResult();

            if (file != juce::File{})                                                // [9]
            {
                auto* reader = formatManager.createReaderFor (file);                 // [10]

                if (reader != nullptr)
                {
                    // Get the number of channels of the audio file
                    int numChannels = reader->numChannels;

                    // if the number of channels is different from 1, aleert the user and return
                    if (numChannels != 1)
					{
						juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "The audio file must be mono", "OK");
						return;
					}

                    auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);   // [11]
                    transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);       // [12]
                    playButton.setEnabled (true);                                                      // [13]
                    readerSource.reset (newSource.release());                                          // [14]

                    String sourceName = file.getFileNameWithoutExtension();
					LoadSource(sourceName, SOURCE1_INITIAL_AZIMUTH, SOURCE1_INITIAL_ELEVATION, SOURCE1_INITIAL_DISTANCE);
                    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Success", "Audio file loaded successfully", "OK");

                }
            }
        });
    }

    void playButtonClicked()
    {
        changeState (Starting);
    }

    void stopButtonClicked()
    {
        changeState (Stopping);
    }


    //==========================================================================
    juce::TextButton openSOFAButton;
    juce::TextButton openWavButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
	juce::Label sourceAzimuthLabel;
    juce::Slider sourceAzimuthDial;
    juce::Label sourceElevationLabel;
    juce::Slider sourceElevationDial;
    juce::Label sourceDistanceLabel;
    juce::Slider sourceDistanceDial;
    juce::OwnedArray<Button> sofaFileButtons;
    juce::Label sampleRateLabel;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState state;

    //==========================================================================
    Common::CGlobalParameters globalParameters;                                   // Global BRT parameters
    BRTBase::CBRTManager brtManager;                                              // BRT global manager interface
    std::shared_ptr<BRTListenerModel::CListenerHRTFbasedModel> listener;          // Pointer to listener model
    std::shared_ptr<BRTSourceModel::CSourceSimpleModel> source1BRT;               // Pointer to audio source model
    float sourceAzimuth{ SOURCE1_INITIAL_AZIMUTH };
    float sourceElevation{ SOURCE1_INITIAL_ELEVATION };
    float sourceDistance{ SOURCE1_INITIAL_DISTANCE };
    BRTReaders::CSOFAReader sofaReader;                                           // SOFA reader provided by BRT Library
    std::vector<std::shared_ptr<BRTServices::CHRTF>> HRTF_list;                   // List of HRTFs loaded

    int selectedHRTFidx;
    HRTFState hrtfState;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
