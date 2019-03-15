/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2017 - ROLI Ltd.

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
//Todo:--------List of things to do-------------------------
// - Add sound files to be processed. Look at the openButtonClicked function
// - Create the gui for the app and create state variables for the different options
// - Load the cipic wav impulses to be convolved to process sounds (I've been looking into this)
//      Have not found an easy solution to this yet. If we can convolve a sound with any kind
//      of impulse then that is a first step. Look into convolution and sound and Juce
// - Figure out a positioning algorithm for players
// - List of resources about hrtfs, convolution, etc:
// - https://docs.google.com/document/d/1rVlQcefDPTmIzDN4WyAqNV8uTuK5Alw5YN2BCIXHmW8/edit?usp=sharing


#pragma once

template <typename Type>
class SoundSim{
public:
SoundSim(){

}

template<typename ProcessContext>
//==============================================================================
void prepare(const juce::dsp::ProcessSpec &spec) {
    processorChain.prepare(spec);
}

//==============================================================================
template<typename ProcessContext>
void process(const ProcessContext &context) noexcept {
    processorChain.process(context);
}

void reset() noexcept {
    processorChain.reset();
}
private:
    juce::dsp::ProcessorChain<juce::dsp::Convolution> processorChain;
    enum
    {
        convolutionIndex
    };
};

//==============================================================================
class MainContentComponent   : public AudioAppComponent,
                               public ChangeListener,
                               public Timer
{
public:
    MainContentComponent()
        :   state (Stopped)
    {

        addAndMakeVisible (&playButton);
        playButton.setButtonText ("Play");
        playButton.onClick = [this] { playButtonClicked(); };
        playButton.setColour (TextButton::buttonColourId, Colours::green);
        playButton.setEnabled (false);

        addAndMakeVisible (&stopButton);
        stopButton.setButtonText ("Stop");
        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setColour (TextButton::buttonColourId, Colours::red);
        stopButton.setEnabled (false);

        addAndMakeVisible (&loopingToggle);
        loopingToggle.setButtonText ("Loop");
        loopingToggle.onClick = [this] { loopButtonChanged(); };

        addAndMakeVisible (&currentPositionLabel);
        currentPositionLabel.setText ("Stopped", dontSendNotification);

        setSize (300, 200);

        formatManager.registerBasicFormats();
        transportSource.addChangeListener (this);

        setAudioChannels (2, 2);
        startTimer (20);
    }

    ~MainContentComponent()
    {
        shutdownAudio();
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        lastSampleRate = sampleRate;
        loadFileToTransport();
        impulseProcessing();
        transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
    }

    //Buffer to fill is
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        if (readerSource.get() == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }

        transportSource.getNextAudioBlock (bufferToFill);
    }

    void releaseResources() override
    {
        transportSource.releaseResources();
    }

    //Todo: Place Buttons and Make Gui suitable for App
    //-Add necessary buttons and options
    //-Place in appropriate places
    void resized() override
    {
        openButton          .setBounds (10, 10,  getWidth() - 20, 20);
        playButton          .setBounds (10, 40,  getWidth() - 20, 20);
        stopButton          .setBounds (10, 70,  getWidth() - 20, 20);
        loopingToggle       .setBounds (10, 100, getWidth() - 20, 20);
        currentPositionLabel.setBounds (10, 130, getWidth() - 20, 20);
    }

    void changeListenerCallback (ChangeBroadcaster* source) override
    {
        if (source == &transportSource)
        {
            if (transportSource.isPlaying())
                changeState (Playing);
            else
                changeState (Stopped);
        }
    }

    void timerCallback() override
    {
        if (transportSource.isPlaying())
        {
            RelativeTime position (transportSource.getCurrentPosition());

            auto minutes = ((int) position.inMinutes()) % 60;
            auto seconds = ((int) position.inSeconds()) % 60;
            auto millis  = ((int) position.inMilliseconds()) % 1000;

            auto positionString = String::formatted ("%02d:%02d:%03d", minutes, seconds, millis);

            currentPositionLabel.setText (positionString, dontSendNotification);
        }
        else
        {
            currentPositionLabel.setText ("Stopped", dontSendNotification);
        }
    }

    void updateLoopState (bool shouldLoop)
    {
        if (readerSource.get() != nullptr)
            readerSource->setLooping (shouldLoop);
    }

private:
    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Stopping
    };

    void changeState (TransportState newState)
    {
        if (state != newState)
        {
            state = newState;

            switch (state)
            {
                case Stopped:
                    stopButton.setEnabled (false);
                    playButton.setEnabled (true);
                    transportSource.setPosition (0.0);
                    break;

                case Starting:
                    playButton.setEnabled (false);
                    transportSource.start();
                    break;

                case Playing:
                    stopButton.setEnabled (true);
                    break;

                case Stopping:
                    transportSource.stop();
                    break;
            }
        }
    }

    void loadFileToTransport(){
        AudioFormat *audioFormat = formatManager.getDefaultFormat();

        auto dir = File::getCurrentWorkingDirectory();
        int numTries = 0;

        //find the resources dir
        while (! dir.getChildFile ("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        File temp = File(dir.getChildFile ("Resources").getChildFile("piano.wav"));
        //std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(temp));
        auto* reader = formatManager.createReaderFor (temp);

        if (reader != nullptr)
        {
            std::unique_ptr<AudioFormatReaderSource> newSource (new AudioFormatReaderSource (reader, true));
            transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);
            playButton.setEnabled (true);
            readerSource.reset (newSource.release());
        }
    }

    void loadFileToProcess() {
        AudioFormat *audioFormat = formatManager.getDefaultFormat();

        auto dir = File::getCurrentWorkingDirectory();
        int numTries = 0;

        //find the resources dir
        while (!dir.getChildFile("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        File temp = File(dir.getChildFile("Resources").getChildFile("piano.wav"));
        //std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(temp));
        auto *reader = formatManager.createReaderFor(temp);

        //Read the File into Left and right buffers
        if (reader != nullptr) {
            auto duration = reader->lengthInSamples / reader->sampleRate;

            pianoBufferL.setSize(1, (int) reader->lengthInSamples);
            reader->read(&pianoBufferL, 0, (int) reader->lengthInSamples, 0, true, true);

            pianoBufferR.setSize(1, (int) reader->lengthInSamples);
            reader->read(&pianoBufferR, 0, (int) reader->lengthInSamples, 0, true, true);
            }
    }


    void playButtonClicked()
    {
        updateLoopState (loopingToggle.getToggleState());
        changeState (Starting);
    }

    void stopButtonClicked()
    {
        changeState (Stopping);
    }

    void loopButtonChanged()
    {
        updateLoopState (loopingToggle.getToggleState());
    }

    void impulseProcessing(){
        loadConvolutionFiles();

        //--------------------Loading Convolutions-------------------------------------------------
        auto& convolutionL = processorChain.template get<convolutionIndex>();
        convolutionL.copyAndLoadImpulseResponseFromBuffer(leftZero, sampleRate, false, true, false, 0);

        auto& convolutionR = processorChain.template get<convolutionIndex>();
        convolutionL.copyAndLoadImpulseResponseFromBuffer(rightZero, sampleRate, false, true, false, 0);

        //Todo Process the piano sample with convolution

    }

    void loadConvolutionFiles(){
        AudioSampleBuffer sampleBuffer;
        int position = 0;
        AudioFormatManager formatManager1;
        formatManager1.registerBasicFormats();

        AudioFormat *audioFormat = formatManager1.getDefaultFormat();

        auto dir = File::getCurrentWorkingDirectory();
        int numTries = 0;

        //find the resources dir
        while (! dir.getChildFile ("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        File left0 = File(dir.getChildFile ("Resources").getChildFile("HRIR").getChildFile("0azleft.wav"));
        File right0 = File(dir.getChildFile ("Resources").getChildFile("HRIR").getChildFile("0azright.wav"));
        std::unique_ptr<AudioFormatReader> readerLeft(formatManager1.createReaderFor(left0));
        std::unique_ptr<AudioFormatReader> readerRight(formatManager1.createReaderFor(left0));

        //Load Left HRIR
        if (readerLeft.get() != nullptr) {
            auto duration = readerLeft->lengthInSamples / readerLeft->sampleRate;

            sampleBuffer.setSize(readerLeft->numChannels, (int) readerRight->lengthInSamples);
            readerLeft->read(&sampleBuffer, 0, (int) readerLeft->lengthInSamples, 0, true, true);
        }

        //Load Right HRIR
        if (readerRight.get() != nullptr) {
            auto duration = readerRight->lengthInSamples / readerRight->sampleRate;

            sampleBuffer.setSize(readerRight->numChannels, (int) readerRight->lengthInSamples);
            readerRight->read(&sampleBuffer, 0, (int) readerRight->lengthInSamples, 0, true, true);

        }

        if(readerRight != nullptr) {
            std::cout << "File loaded HRIR Right into audio buffer\n";
            std::cout << "Audio Buffer #Samples:" << sampleBuffer.getNumSamples() << "\n";
            std::cout << "Audio Buffer #Channels:" << sampleBuffer.getNumChannels() << "\n";
        }

        if(readerLeft != nullptr) {
            std::cout << "File loaded HRIR Right into audio buffer\n";
            std::cout << "Audio Buffer #Samples:" << sampleBuffer.getNumSamples() << "\n";
            std::cout << "Audio Buffer #Channels:" << sampleBuffer.getNumChannels() << "\n";
        }

        //--------------------Reshape the HRIR to the correct shape-------------------------
        rightZero.setSize(1, (int)200);
        int count = 0;
        while(count++ < 199) {

            //Reallocate the buffer to the correct shape
            rightZero.setSample(0, count, sampleBuffer.getSample(count, 9));
            std::cout << rightZero.getSample(0, count) << "\n";

        }

        leftZero.setSize(1, (int)200);
        count = 0;
        while(count++ < 199) {

            //Reallocate the buffer to the correct shape
            leftZero.setSample(0, count, sampleBuffer.getSample(count, 9));
            std::cout << leftZero.getSample(0, count) << "\n";

        }

        //Create a new buffer form the data stored
        std::cout << "New Created audio buffer of HRIR Right\n";
        std::cout << "Audio Buffer #Samples:" << rightZero.getNumSamples() << "\n";
        std::cout << "Audio Buffer #Channels:" << rightZero.getNumChannels() << "\n";

        //Create a new buffer form the data stored
        std::cout << "New Created audio buffer of HRIR Left\n";
        std::cout << "Audio Buffer #Samples:" << leftZero.getNumSamples() << "\n";
        std::cout << "Audio Buffer #Channels:" << leftZero.getNumChannels() << "\n";
    }

    //==========================================================================
    TextButton openButton;
    TextButton playButton;
    TextButton stopButton;
    ToggleButton loopingToggle;
    Label currentPositionLabel;

    double lastSampleRate;
    AudioFormatManager formatManager;
    std::unique_ptr<AudioFormatReaderSource> readerSource;
    AudioTransportSource transportSource;
    TransportState state;

    //This is where the processing of the convolution will occur.
    juce::dsp::ProcessorChain<juce::dsp::Convolution> processorChain;
    enum
    {
        convolutionIndex
    };

    AudioFormatManager formatManager1;
    AudioSampleBuffer pianoBufferL;
    AudioSampleBuffer pianoBufferR;
    AudioSampleBuffer rightZero;
    AudioSampleBuffer leftZero;
    double sampleRate = 44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};