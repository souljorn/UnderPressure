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
AudioSampleBuffer rightZero;
AudioSampleBuffer leftZero;
//==============================================================================
class ProcessorBase  : public AudioProcessor
{
public:
    //==============================================================================
    ProcessorBase()  {}
    ~ProcessorBase() {}

    //==============================================================================
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override {}

    //==============================================================================
    AudioProcessorEditor* createEditor() override          { return nullptr; }
    bool hasEditor() const override                        { return false; }

    //==============================================================================
    const String getName() const override                  { return {}; }
    bool acceptsMidi() const override                      { return false; }
    bool producesMidi() const override                     { return false; }
    double getTailLengthSeconds() const override           { return 0; }

    //==============================================================================
    int getNumPrograms() override                          { return 0; }
    int getCurrentProgram() override                       { return 0; }
    void setCurrentProgram (int) override                  {}
    const String getProgramName (int) override             { return {}; }
    void changeProgramName (int, const String&) override   {}

    //==============================================================================
    void getStateInformation (MemoryBlock&) override       {}
    void setStateInformation (const void*, int) override   {}

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorBase)
};

//==============================================================================
class FilterProcessor  : public ProcessorBase
{

public:
    FilterProcessor() {}

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        *filter.state = *dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 8000.0f);

        dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), 2 };
        filter.prepare (spec);
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer&) override
    {
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> context (block);
        filter.process (context);
    }

    void reset() override
    {
        filter.reset();
    }

    const String getName() const override { return "Filter"; }

private:
    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> filter;
};


class ConProcessorLeft  : public ProcessorBase
{

public:
    ConProcessorLeft() {}

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {

        //--------------------Loading Convolutions-------------------------------------------------
        auto& convolutionL = convolution.template get<convolutionIndex>();
        convolutionL.copyAndLoadImpulseResponseFromBuffer(leftZero, sampleRate, true, false, false, 0);
        dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), 1 };
        convolution.prepare (spec);
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer&) override
    {
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> context (block);
        convolution.process (context);
    }

    void reset() override
    {
        convolution.reset();
    }

    const String getName() const override { return "Convolution"; }

private:
    dsp::ProcessorChain<juce::dsp::Convolution> convolution;

    enum
    {
        convolutionIndex
    };
};

class ConProcessorRight  : public ProcessorBase
{

public:
    ConProcessorRight() {}

    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {

        //--------------------Loading Convolutions-------------------------------------------------
        auto& convolutionL = convolution.template get<convolutionIndex>();
        convolutionL.copyAndLoadImpulseResponseFromBuffer(leftZero, sampleRate, false, true, false, 0);
        dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), 1 };
        convolution.prepare (spec);
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer&) override
    {
        dsp::AudioBlock<float> block (buffer);
        dsp::ProcessContextReplacing<float> context (block);
        convolution.process (context);
    }

    void reset() override
    {
        convolution.reset();
    }

    const String getName() const override { return "Convolution"; }

private:
    dsp::ProcessorChain<juce::dsp::Convolution> convolution;

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
        //loadFileToProcess();
        impulseProcessing();
        blockSize = samplesPerBlockExpected;
        //conProcessorRight.prepareToPlay(sampleRate,samplesPerBlockExpected);
        //conProcessorLeft.prepareToPlay(sampleRate,samplesPerBlockExpected);
        //filter.prepareToPlay(sampleRate,samplesPerBlockExpected);
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
        conProcessorLeft.processBlock(*bufferToFill.buffer, emptyMidi);

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

        File temp = File(dir.getChildFile ("Resources").getChildFile("Chuff.wav"));
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

void loadConvolutionFiles() {
    AudioSampleBuffer sampleBuffer;
    int position = 0;
    AudioFormatManager formatManager1;
    formatManager1.registerBasicFormats();
    AudioFormat *audioFormat = formatManager1.getDefaultFormat();
    auto dir = File::getCurrentWorkingDirectory();
    int numTries = 0;
    //find the resources dir
    while (!dir.getChildFile("Resources").exists() && numTries++ < 15) {
        dir = dir.getParentDirectory();
    }

    for (int i = 0; i <= 80; i += 5) {
        String fileR = "";
        fileR += i;
        fileR += "azright.wav";
        std::cout << fileR;
        String fileL = "";
        fileL += i;
        fileL += "azleft.wav";
        std::cout << fileL;
        String negR = "neg";
        negR += i;
        negR += "azright.wav";
        String negL = "neg";
        negL += i;
        negL += "azleft.wav";
        File fileLeft = File(dir.getChildFile("Resources").getChildFile("HRIR").getChildFile(fileL));
        File fileRight = File(dir.getChildFile("Resources").getChildFile("HRIR").getChildFile(fileR));
        File negLeft = File(dir.getChildFile("Resources").getChildFile("HRIR").getChildFile(negL));
        File negRight = File(dir.getChildFile("Resources").getChildFile("HRIR").getChildFile(negR));
        std::unique_ptr<AudioFormatReader> readerLeft(formatManager1.createReaderFor(fileLeft));
        std::unique_ptr<AudioFormatReader> readerRight(formatManager1.createReaderFor(fileRight));
        std::unique_ptr<AudioFormatReader> readerNegLeft(formatManager1.createReaderFor(negLeft));
        std::unique_ptr<AudioFormatReader> readerNegRight(formatManager1.createReaderFor(negRight));

        //Load Left HRIR
        if (readerLeft.get() != nullptr) {
            auto duration = readerLeft->lengthInSamples / readerLeft->sampleRate;
            sampleBuffer.setSize(readerLeft->numChannels, (int)readerRight->lengthInSamples);
            readerLeft->read(&sampleBuffer, 0, (int)readerLeft->lengthInSamples, 0, true, true);
        }
        //Load Right HRIR
        if (readerRight.get() != nullptr) {
            auto duration = readerRight->lengthInSamples / readerRight->sampleRate;
            sampleBuffer.setSize(readerRight->numChannels, (int)readerRight->lengthInSamples);
            readerRight->read(&sampleBuffer, 0, (int)readerRight->lengthInSamples, 0, true, true);
        }
        if (readerRight != nullptr) {
            std::cout << "----------------------------------------------------"  << "\n" ;
            std::cout << "File loaded HRIR Right into audio buffer for:" << i  << "\n" ;
            std::cout << "Audio Buffer #Samples:" << sampleBuffer.getNumSamples() << "\n";
            std::cout << "Audio Buffer #Channels:" << sampleBuffer.getNumChannels() << "\n";
        }
        if (readerLeft != nullptr) {
            std::cout << "File loaded HRIR Right into audio buffer\n";
            std::cout << "Audio Buffer #Samples:" << sampleBuffer.getNumSamples() << "\n";
            std::cout << "Audio Buffer #Channels:" << sampleBuffer.getNumChannels() << "\n";
        }

        //--------------------Reshape the HRIR to the correct shape-------------------------
        rightZero.setSize(1, (int)200);
        int count = 0;
        while (count++ < 199) {
            //Reallocate the buffer to the correct shape
            rightZero.setSample(0, count, sampleBuffer.getSample(count, 8));
            std::cout << rightZero.getSample(0, count) << "\n";
        }

        leftZero.setSize(1, (int)200);
        count = 0;
        while (count++ < 199) {
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
        //push the value into the back of the vector for each iteration of the for loop (+5)
        //ex: 0 is stored in [0], 5 is stored in [1], etc.
        rightVec.push_back(rightZero);
        leftVec.push_back(leftZero);
    }
    }


//    void loadConvolutionFiles(){
//        AudioSampleBuffer sampleBuffer;
//        int position = 0;
//        AudioFormatManager formatManager1;
//        formatManager1.registerBasicFormats();
//
//        AudioFormat *audioFormat = formatManager1.getDefaultFormat();
//
//        auto dir = File::getCurrentWorkingDirectory();
//        int numTries = 0;
//
//        //find the resources dir
//        while (! dir.getChildFile ("Resources").exists() && numTries++ < 15) {
//            dir = dir.getParentDirectory();
//        }
//
//        File left0 = File(dir.getChildFile ("Resources").getChildFile("HRIR").getChildFile("80azleft.wav"));
//        File right0 = File(dir.getChildFile ("Resources").getChildFile("HRIR").getChildFile("80azright.wav"));
//        std::unique_ptr<AudioFormatReader> readerLeft(formatManager1.createReaderFor(left0));
//        std::unique_ptr<AudioFormatReader> readerRight(formatManager1.createReaderFor(left0));
//
//
//        //Load Left HRIR
//        if (readerLeft.get() != nullptr) {
//            auto duration = readerLeft->lengthInSamples / readerLeft->sampleRate;
//
//            sampleBuffer.setSize(readerLeft->numChannels, (int) readerRight->lengthInSamples);
//            readerLeft->read(&sampleBuffer, 0, (int) readerLeft->lengthInSamples, 0, true, true);
//        }
//
//        //Load Right HRIR
//        if (readerRight.get() != nullptr) {
//            auto duration = readerRight->lengthInSamples / readerRight->sampleRate;
//
//            sampleBuffer.setSize(readerRight->numChannels, (int) readerRight->lengthInSamples);
//            readerRight->read(&sampleBuffer, 0, (int) readerRight->lengthInSamples, 0, true, true);
//
//        }
//
//        if(readerRight != nullptr) {
//            std::cout << "File loaded HRIR Right into audio buffer\n";
//            std::cout << "Audio Buffer #Samples:" << sampleBuffer.getNumSamples() << "\n";
//            std::cout << "Audio Buffer #Channels:" << sampleBuffer.getNumChannels() << "\n";
//        }
//
//        if(readerLeft != nullptr) {
//            std::cout << "File loaded HRIR Right into audio buffer\n";
//            std::cout << "Audio Buffer #Samples:" << sampleBuffer.getNumSamples() << "\n";
//            std::cout << "Audio Buffer #Channels:" << sampleBuffer.getNumChannels() << "\n";
//        }
//
//        //--------------------Reshape the HRIR to the correct shape-------------------------
//        rightZero.setSize(1, (int)200);
//        int count = 0;
//        while(count++ < 199) {
//
//            //Reallocate the buffer to the correct shape
//            rightZero.setSample(0, count, sampleBuffer.getSample(count, 9));
//            std::cout << rightZero.getSample(0, count) << "\n";
//
//        }
//
//        leftZero.setSize(1, (int)200);
//        count = 0;
//        while(count++ < 199) {
//
//            //Reallocate the buffer to the correct shape
//            leftZero.setSample(0, count, sampleBuffer.getSample(count, 9));
//            std::cout << leftZero.getSample(0, count) << "\n";
//        }
//
//        //Create a new buffer form the data stored
//        std::cout << "New Created audio buffer of HRIR Right\n";
//        std::cout << "Audio Buffer #Samples:" << rightZero.getNumSamples() << "\n";
//        std::cout << "Audio Buffer #Channels:" << rightZero.getNumChannels() << "\n";
//
//        //Create a new buffer form the data stored
//        std::cout << "New Created audio buffer of HRIR Left\n";
//        std::cout << "Audio Buffer #Samples:" << leftZero.getNumSamples() << "\n";
//        std::cout << "Audio Buffer #Channels:" << leftZero.getNumChannels() << "\n";
//    }

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

    ConProcessorRight conProcessorRight;
    ConProcessorLeft conProcessorLeft;

    FilterProcessor filter;
    AudioFormatManager formatManager1;
    AudioSampleBuffer pianoBufferL;
    AudioSampleBuffer pianoBufferR;
    AudioSampleBuffer * tempL;
    AudioSampleBuffer * tempR;

    AudioSampleBuffer filterBuffer;
    double sampleRate = 44100.0;
    MidiBuffer emptyMidi;
    int blockSize;
    std::vector<AudioSampleBuffer> rightVec;
    std::vector<AudioSampleBuffer> leftVec;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};