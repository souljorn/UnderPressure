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

AudioSampleBuffer leftChannel;
AudioSampleBuffer rightChannel;
struct AudioPlayer{
    int playHead;
    AudioSampleBuffer buffer;
    AudioSampleBuffer convolvedBuffer;
    int elevation;
    int azimuth;


    AudioPlayer(AudioSampleBuffer buffer) : buffer(buffer){

    }

    AudioPlayer(AudioSampleBuffer buffer, int elv, int az) : buffer(buffer), azimuth(az), elevation(elv){

    }
};
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
        *filter.state = *dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 5000.0f);

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
    ConProcessorLeft() {
        irBuffer = leftZero;
    }
    AudioSampleBuffer irBuffer;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        std::cout << "---------------Reload HRIR------------------->\n";
        //--------------------Loading Convolutions-------------------------------------------------
        //auto& convolutionL = convolution.template get<convolutionIndex>();
        convolution.copyAndLoadImpulseResponseFromBuffer(irBuffer, sampleRate, true, true, true, irBuffer.getNumSamples());
        dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), 2 };
        convolution.prepare (spec);
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer&) override
    {

        //Make mono
        float * buffLeft = buffer.getWritePointer(0,0);
        for(int i =0; i < buffer.getNumSamples(); ++i){
            buffLeft[i] = buffer.getSample(0,i);
        }

        float * buffRight = buffer.getWritePointer(1,0);
        for(int i =0; i < buffer.getNumSamples(); ++i){
            buffRight[i] = buffer.getSample(0,i);
        }

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
    juce::dsp::Convolution convolution;

};

class ConProcessorRight  : public ProcessorBase
{

public:
    ConProcessorRight() {
        irBuffer = rightZero;
    }
    AudioSampleBuffer irBuffer;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {

        //--------------------Loading Convolutions-------------------------------------------------
        convolution.copyAndLoadImpulseResponseFromBuffer(irBuffer, sampleRate, true, true, true, irBuffer.getNumSamples());
        dsp::ProcessSpec spec { sampleRate, static_cast<uint32> (samplesPerBlock), 2 };
        convolution.prepare (spec);
    }

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer&) override
    {
        //Make mono
        float * buffLeft = buffer.getWritePointer(0,0);
        for(int i =0; i < buffer.getNumSamples(); ++i){
            buffLeft[i] = buffer.getSample(0,i);
        }

        float * buffRight = buffer.getWritePointer(1,0);
        for(int i =0; i < buffer.getNumSamples(); ++i){
            buffRight[i] = buffer.getSample(0,i);
        }

        //std::cout << "Num Channels" << buffer.getNumChannels() << "\n";
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
    juce::dsp::Convolution convolution;
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
        playButton.setEnabled (true);

        addAndMakeVisible (&stopButton);
        stopButton.setButtonText ("Stop");
        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setColour (TextButton::buttonColourId, Colours::red);
        stopButton.setEnabled (false);

        addAndMakeVisible (&loopingToggle);
        loopingToggle.setButtonText ("Loop");
        loopingToggle.onClick = [this] { loopButtonChanged(); };
        loopingToggle.setEnabled(true);

        addAndMakeVisible (&currentPositionLabel);
        currentPositionLabel.setText ("Stopped", dontSendNotification);

        setSize (300, 200);

        formatManager.registerBasicFormats();
        transportSource.addChangeListener (this);

        //std::unique_ptr<AudioTransportSource> tempPtr(new AudioTransportSource());
        //tempPtr->addChangeListener(this);
        //audioSources.push_back(tempPtr);

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
        samplesExpected = samplesPerBlockExpected;

        tempL = std::unique_ptr<AudioSampleBuffer>(new AudioSampleBuffer);
        tempR = std::unique_ptr<AudioSampleBuffer>(new AudioSampleBuffer);
        inputL = std::unique_ptr<AudioSampleBuffer>(new AudioSampleBuffer);
        inputR = std::unique_ptr<AudioSampleBuffer>(new AudioSampleBuffer);

        loadFileToTransport();
//        loadAudioFile("PlayerLoopMono.wav");
//        loadAudioFile("crowd1.wav");
//        loadAudioFile("crowd2.wav");
        loadAudioFile("cello.wav");
        impulseProcessing();
        blockSize = samplesPerBlockExpected;
        conProcessorLeft = std::unique_ptr<ConProcessorLeft>(new ConProcessorLeft);
        conProcessorRight = std::unique_ptr<ConProcessorRight>(new ConProcessorRight);

        conProcessorRight->prepareToPlay(samplesPerBlockExpected, sampleRate);
        conProcessorLeft->prepareToPlay(samplesPerBlockExpected, sampleRate);

        filter.prepareToPlay(sampleRate,samplesPerBlockExpected);
        transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);

        audioList.at(0).convolvedBuffer = placeSound(4, audioList.at(0).buffer);
        std::cout << "num Channels " << audioList.at(0).buffer.getNumChannels() << "\n";
        std::cout << "num Channels convolved " << audioList.at(0).convolvedBuffer.getNumChannels() << "\n";
    }

    //Buffer to fill
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {

//        if (readerSource.get() == nullptr)
//        {
//            bufferToFill.clearActiveBufferRegion();
//            return;
//        }

        //Get Audio loaded from transport source
        //transportSource.getNextAudioBlock (bufferToFill);
        //audioSources.at(0)->getNextAudioBlock(bufferToFill);
        //std::cout << audioBuffers.at(0).getNumSamples() << std::endl;

        //Get a preprocessed verstion stored called inputL and inputR
//        inputL->setSize(1,bufferToFill.numSamples);
////        inputR->setSize(1,bufferToFill.numSamples);
////
////        for(int i =0; i < bufferToFill.numSamples; ++i){
////            inputL->setSample(0, i, bufferToFill.buffer->getSample(0,i) + audioList.at(0).buffer.getSample(0, audioList.at(0).playHead));
////            inputR->setSample(0, i, bufferToFill.buffer->getSample(1,i) + audioList.at(0).buffer.getSample(0, audioList.at(0).playHead++));
////            audioList.at(0).playHead %= audioList.at(0).buffer.getNumSamples();
////            //std::cout << bufferToFill.buffer->getSample(1,i) << std::endl;
////        }
////
////        //Reset input buffer to original state
////        float * buffLeft = bufferToFill.buffer->getWritePointer(0,bufferToFill.startSample);
////        for(int i =0; i < bufferToFill.numSamples; ++i){
////            buffLeft[i] = inputL->getSample(0,i);
////        }
////
////        float * buffRight = bufferToFill.buffer->getWritePointer(1,bufferToFill.startSample);
////        for(int i =0; i < bufferToFill.numSamples; ++i){
////            buffRight[i] = inputR->getSample(0,i);
////        }


            addAudioBuffers(bufferToFill, audioList.at(0));

            //addAudioBuffers(bufferToFill, audioList.at(1));

//
//        //Perform convolution on left channel and store processed convolution in a temp buffer
//        conProcessorLeft->processBlock(*bufferToFill.buffer, emptyMidi);
//        tempL->setSize(1,bufferToFill.numSamples);
//        for(int i =0; i < bufferToFill.numSamples; ++i){
//            tempL->setSample(0, i, bufferToFill.buffer->getSample(0,i));
//        }
//
//        //Reset input buffer to original state
//        float * buffLeft = bufferToFill.buffer->getWritePointer(0,bufferToFill.startSample);
//        for(int i =0; i < bufferToFill.numSamples; ++i){
//            buffLeft[i] = inputL->getSample(0,i);
//        }
//
//        float * buffRight = bufferToFill.buffer->getWritePointer(1,bufferToFill.startSample);
//        for(int i =0; i < bufferToFill.numSamples; ++i){
//            buffRight[i] = inputR->getSample(0,i);
//        }
//
//        //Perform convolution on right channel
//        conProcessorRight->processBlock( *bufferToFill.buffer, emptyMidi);
//
//        //Overwrite left channel with left convolved buffer
//        for(int i =0; i < bufferToFill.numSamples; ++i){
//            buffLeft[i] = tempL->getSample(0,i);
//        }
//
//        //Add filter to rear HRIR
//        //if(impulseIndex > 17 && impulseIndex < 50)
//           //  filter.processBlock(*bufferToFill.buffer,emptyMidi);
//
//        //Reloading of HRIR every time period
//        (relativeTime += relativeTime.milliseconds(10)).inMilliseconds();
//        if(relativeTime.inMilliseconds() > 500.0f){
//            std::cout << "Approximate Azimuth Angle: " << degrees << "\n";
//            degrees += 5;
//            relativeTime = relativeTime.milliseconds(0);
//
//            //Reset convolution processes
//            conProcessorLeft->reset();
//            conProcessorRight->reset();
//
//            //Write new hrir for convolution at new angle
//            float * irWriteLeft =conProcessorLeft->irBuffer.getWritePointer(0);
//            float * irWriteRight =conProcessorRight->irBuffer.getWritePointer(0);
//            for(int i = 0; i < 200; i++){
//                irWriteLeft[i] = leftHRIR.at(impulseIndex).getSample(0,i);
//                irWriteRight[i] = rightHRIR.at(impulseIndex).getSample(0,i);
//            }
//            impulseIndex++;
//
//            //Reset angle to 0
//            if(impulseIndex > rightHRIR.size() -1) {
//                impulseIndex = 0;
//                degrees = 0;
//            }
//
//            //prepare convolution processors
//            conProcessorLeft->prepareToPlay(sampleRate, samplesExpected);
//            conProcessorRight->prepareToPlay(sampleRate, samplesExpected);
//        }
    }

    void addAudioBuffers(const AudioSourceChannelInfo& source, AudioPlayer &toAdd){
        inputL->setSize(1,source.numSamples);
        inputR->setSize(1,source.numSamples);

        for(int i =0; i < source.numSamples; ++i){
            inputL->setSample(0, i, source.buffer->getSample(0,i) + toAdd.buffer.getSample(0, toAdd.playHead));
            inputR->setSample(0, i, source.buffer->getSample(1,i) + toAdd.buffer.getSample(1, toAdd.playHead++));
            toAdd.playHead %= toAdd.buffer.getNumSamples();
            //std::cout << bufferToFill.buffer->getSample(1,i) << std::endl;
        }

        //Reset input buffer to original state
        float * buffLeft = source.buffer->getWritePointer(0,source.startSample);
        for(int i =0; i < source.numSamples; ++i){
            buffLeft[i] = inputL->getSample(0,i);
            //buffLeft[i] = 0;
        }

        float * buffRight = source.buffer->getWritePointer(1,source.startSample);
        for(int i =0; i < source.numSamples; ++i){
            buffRight[i] = inputR->getSample(0,i);
            //buffRight[i] = 0;
        }
    }

   AudioSampleBuffer placeSound(int index, AudioSampleBuffer & inputBuffer){
        AudioSampleBuffer leftBuffer;
        AudioSampleBuffer rightBuffer;

        //Write new hrir for convolution at new angle
        float * irWriteLeft =conProcessorLeft->irBuffer.getWritePointer(0);
        float * irWriteRight =conProcessorRight->irBuffer.getWritePointer(0);
        for(int i = 0; i < 200; i++){
            irWriteLeft[i] = leftHRIR.at(index).getSample(0,i);
            irWriteRight[i] = rightHRIR.at(index).getSample(0,i);
        }
        //Reset convolution processes
        conProcessorLeft->reset();
        conProcessorRight->reset();

        //prepare convolution processors
        conProcessorLeft->prepareToPlay(sampleRate, samplesExpected);
        conProcessorRight->prepareToPlay(sampleRate, samplesExpected);

        inputL->setSize(1,inputBuffer.getNumSamples());
        inputR->setSize(1,inputBuffer.getNumSamples());

        for(int i =0; i < inputBuffer.getNumSamples(); ++i){
            inputL->setSample(0, i, inputBuffer.getSample(0,i));
            inputR->setSample(0, i, inputBuffer.getSample(0,i));

            //std::cout << "Left: " << inputL->getSample(0,i) << std::endl;
            //std::cout << "Right: " << inputR->getSample(0,i) << std::endl;
        }

        //ProcessLeft
        conProcessorLeft->processBlock( inputBuffer, emptyMidi);

        leftBuffer.setSize(1, inputBuffer.getNumSamples());
        //Save and Reset input Buffer
        for(int i =0; i < inputBuffer.getNumSamples(); ++i){
            leftBuffer.setSample(0,i,inputBuffer.getSample(0,i));
            inputBuffer.setSample(0, i, inputL->getSample(0,i));
            inputBuffer.setSample(1, i, inputL->getSample(0,i));
        }

        //ProcessRight
        conProcessorRight->processBlock( inputBuffer, emptyMidi);

        //Overwrite left channel with left convolved buffer
        for(int i =0; i < inputBuffer.getNumSamples(); ++i){
            inputBuffer.setSample(0, i,  leftBuffer.getSample(0,i));
        }

       std::cout << "input Left: " << inputBuffer.getSample(0,1000) << std::endl;
       std::cout << "input Right: " << inputBuffer.getSample(1,1000) << std::endl;

       std::cout << inputBuffer.getNumChannels() << " input num channels\n";
       return inputBuffer;

    }

    void releaseResources() override
    {
        transportSource.releaseResources();
        conProcessorLeft->releaseResources();
        conProcessorRight->releaseResources();
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

        //File temp = File(dir.getChildFile ("Resources").getChildFile("BasketballFeet.wav"));
        File temp = File(dir.getChildFile ("Resources").getChildFile("PlayerLoopMonoWithVoice.wav"));
//        File temp = File(dir.getChildFile ("Resources").getChildFile("SodaCan.wav"));
       // File temp = File(dir.getChildFile ("Resources").getChildFile("PlayerMonoWhistle.wav"));
        File temp1 = File(dir.getChildFile ("Resources").getChildFile("Register.wav"));
        //std::unique_ptr<AudioFormatReader> reader(formatManager.createReaderFor(temp));
        auto* reader = formatManager.createReaderFor (temp);
        auto* reader2 = formatManager.createReaderFor (temp1);

        if (reader != nullptr)
        {
            std::unique_ptr<AudioFormatReaderSource> newSource (new AudioFormatReaderSource (reader, true));
            transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);
            playButton.setEnabled (true);
            readerSource.reset (newSource.release());
            std::cout << "AudioFile Loaded! \n";
        }
//        if (reader2 != nullptr)
//        {
//            std::unique_ptr<AudioFormatReaderSource> newSource2 (new AudioFormatReaderSource (reader2, true));
//            audioSources.at(0)->setSource (newSource2.get(), 0, nullptr, reader2->sampleRate);
//            playButton.setEnabled (true);
//            readerSource.reset (newSource2.release());
//            std::cout << "AudioFile Loaded! \n";
//        }
    }

    void loadAudioFile(String fileName){
        auto dir = File::getCurrentWorkingDirectory();
        int numTries = 0;
        AudioSampleBuffer sampleBuffer;
        AudioFormatManager formatMan;
        formatMan.registerBasicFormats();
        AudioFormat *audioFormat = formatMan.getDefaultFormat();

        //find the resources dir
        while (! dir.getChildFile ("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        File temp = File(dir.getChildFile ("Resources").getChildFile(fileName));
        while (! dir.getChildFile ("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        std::unique_ptr<AudioFormatReader> source (formatManager.createReaderFor(temp));

        if (source.get() != nullptr) {
            auto duration = source->lengthInSamples / source->sampleRate;
            sampleBuffer.setSize(2, (int)source->lengthInSamples);
            source->read(&sampleBuffer, 0, (int)source->lengthInSamples, 0, true, true);
        }

        audioBuffers.push_back(sampleBuffer);
        AudioPlayer tempAudio(sampleBuffer, 9, 9);
        audioList.push_back(tempAudio);
//        std::cout<< "Reader size:" << source->lengthInSamples << "\n";
//        std::cout<< "Channels:" << source->numChannels << "\n";
//        std::cout<< "Channels:" << sampleBuffer.getNumChannels() << "\n";
//
//        leftChannel.setSize(1, sampleBuffer.getNumSamples());
//        rightChannel.setSize(1, sampleBuffer.getNumSamples());
//        float * buff = leftChannel.getWritePointer(0,0);
//
//
//        //write left and right channels mono
//        for(int i = 0; i < sampleBuffer.getNumSamples(); ++i ){
//            leftChannel.setSample(0,i,sampleBuffer.getSample(0,i));
//            //std::cout<< "LeftChannel sample:" << leftChannel.getSample(0,i) << "\n";
//        }
//        std::cout<< "LeftChannel size:" << leftChannel.getNumSamples() << "\n";
//        std::cout<< "LeftChannel numChannels:" << leftChannel.getNumChannels() << "\n";
//
//        for(int i = 0; i < sampleBuffer.getNumSamples(); ++i ){
//            rightChannel.setSample(0,i,sampleBuffer.getSample(0,i));
//        }
//        std::cout<< "RightChannel size:" << rightChannel.getNumSamples() << "\n";
//        std::cout<< "RightChannel numChannels:" << rightChannel.getNumChannels() << "\n";

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
        loadConvolutionFile();
    }

    void loadConvolutionFiles() {
        //Front Convolutions
        AudioSampleBuffer sampleBufferLeft;
        AudioSampleBuffer sampleBufferRight;
        AudioSampleBuffer sampleBufferLeftNeg;
        AudioSampleBuffer sampleBufferRightNeg;
        AudioSampleBuffer copyL;
        AudioSampleBuffer copyR;
        AudioSampleBuffer copyLNegative;
        AudioSampleBuffer copyRNegative;

        //Behind Convolutions
        AudioSampleBuffer sampleBufferLeftBehind;
        AudioSampleBuffer sampleBufferRightBehind;
        AudioSampleBuffer sampleBufferLeftNegBehind;
        AudioSampleBuffer sampleBufferRightNegBehind;
        AudioSampleBuffer copyLBehind;
        AudioSampleBuffer copyRBehind;
        AudioSampleBuffer copyLNegativeBehind;
        AudioSampleBuffer copyRNegativeBehind;

        //temp vectors
        std::vector<AudioSampleBuffer> positiveBehindRightVec;
        std::vector<AudioSampleBuffer> positiveBehindLeftVec;

        std::vector<AudioSampleBuffer> negativeBehindRightVec;
        std::vector<AudioSampleBuffer> negativeBehindLeftVec;

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
            if(i != 0){
            String negR = "neg";
            negR += i;
            negR += "azright.wav";
            String negL = "neg";
            negL += i;
            negL += "azleft.wav";
                File negLeft = File(dir.getChildFile("Resources").getChildFile("HRIR").getChildFile(negL));
                File negRight = File(dir.getChildFile("Resources").getChildFile("HRIR").getChildFile(negR));
                std::unique_ptr<AudioFormatReader> readerNegLeft(formatManager1.createReaderFor(negLeft));
                std::unique_ptr<AudioFormatReader> readerNegRight(formatManager1.createReaderFor(negRight));
                //-----------------Negative Azimuth readers-----------------------------------
                //Load Left HRIR
                if (readerNegLeft.get() != nullptr) {
                    auto duration = readerNegLeft->lengthInSamples / readerNegLeft->sampleRate;
                    sampleBufferLeftNeg.setSize(readerNegLeft->numChannels, (int)readerNegLeft->lengthInSamples);
                    readerNegLeft->read(&sampleBufferLeftNeg, 0, (int)readerNegLeft->lengthInSamples, 0, true, true);
                }
                //Load Right HRIR
                if (readerNegRight.get() != nullptr) {
                    auto duration = readerNegRight->lengthInSamples / readerNegRight->sampleRate;
                    sampleBufferRightNeg.setSize(readerNegRight->numChannels, (int)readerNegRight->lengthInSamples);
                    readerNegRight->read(&sampleBufferRightNeg, 0, (int)readerNegRight->lengthInSamples, 0, true, true);
                }
            }
            File fileLeft = File(dir.getChildFile("Resources").getChildFile("HRIR").getChildFile(fileL));
            File fileRight = File(dir.getChildFile("Resources").getChildFile("HRIR").getChildFile(fileR));

            std::unique_ptr<AudioFormatReader> readerLeft(formatManager1.createReaderFor(fileLeft));
            std::unique_ptr<AudioFormatReader> readerRight(formatManager1.createReaderFor(fileRight));


            //Load Left HRIR
            if (readerLeft.get() != nullptr) {
                auto duration = readerLeft->lengthInSamples / readerLeft->sampleRate;
                sampleBufferLeft.setSize(readerLeft->numChannels, (int)readerRight->lengthInSamples);
                readerLeft->read(&sampleBufferLeft, 0, (int)readerLeft->lengthInSamples, 0, true, true);
            }
            //Load Right HRIR
            if (readerRight.get() != nullptr) {
                auto duration = readerRight->lengthInSamples / readerRight->sampleRate;
                sampleBufferRight.setSize(readerRight->numChannels, (int)readerRight->lengthInSamples);
                readerRight->read(&sampleBufferRight, 0, (int)readerRight->lengthInSamples, 0, true, true);
            }

            //--------------------Reshape the HRIR to the correct shape-------------------------
            copyR.setSize(1, 200);          //front +
            copyRNegative.setSize(1,200);   //front -
            copyRBehind.setSize(1,200);         //behind +
            copyRNegativeBehind.setSize(1,200); //behind -


            //Front level elevation 9
            //Back level elevation 39
            const int elevationFront = 9;
            const int elevationBehind = 39;
            int count = 0;
            while (count++ < 199) {
                //Reallocate the buffer to the correct shape
                copyR.setSample(0, count, sampleBufferRight.getSample(count, elevationFront));
                copyRBehind.setSample(0,count,sampleBufferRight.getSample(count, elevationBehind));
                if(i!=0) {
                    copyRNegative.setSample(0, count, sampleBufferRightNeg.getSample(count, elevationFront));
                    copyRNegativeBehind.setSample(0, count, sampleBufferRightNeg.getSample(count, elevationBehind));
                }
            }
            std::cout << "\nRMS R Positive"  << copyR.getRMSLevel(0,0, copyR.getNumSamples()) << "\n";
            std::cout << "RMS R Negative Behind"  << copyRNegativeBehind.getRMSLevel(0,0, copyRNegativeBehind.getNumSamples()) << "\n";
            std::cout << "RMS R Positive Behind"  << copyRBehind.getRMSLevel(0,0, copyRBehind.getNumSamples()) << "\n";
            std::cout << "RMS R Negative"  << copyRNegative.getRMSLevel(0,0, copyRNegative.getNumSamples()) << "\n";


            copyL.setSize(1, (int)200);         //front +
            copyLNegative.setSize(1,200);       //front -
            copyLBehind.setSize(1,200);         //behind +
            copyLNegativeBehind.setSize(1,200); //behind -
            count = 0;
            while (count++ < 199) {
                //Reallocate the buffer to the correct shape
                copyL.setSample(0, count, sampleBufferLeft.getSample(count, elevationFront));
                copyLBehind.setSample(0, count, sampleBufferLeft.getSample(count, elevationBehind));
                //disregard the 0th index for negative
                if(i!=0) {
                    copyLNegative.setSample(0, count, sampleBufferLeftNeg.getSample(count, elevationFront));
                    copyLNegativeBehind.setSample(0, count, sampleBufferLeftNeg.getSample(count, elevationBehind));
                }
            }
            std::cout << "\nRMS L Positive"  << copyL.getRMSLevel(0,0, copyL.getNumSamples()) << "\n";
            std::cout << "RMS L Negative Behind"  << copyLNegativeBehind.getRMSLevel(0,0, copyLNegativeBehind.getNumSamples()) << "\n";
            std::cout << "RMS L Positive Behind"  << copyLBehind.getRMSLevel(0,0, copyLBehind.getNumSamples()) << "\n";
            std::cout << "RMS L Negative"  << copyLNegative.getRMSLevel(0,0, copyLNegative.getNumSamples()) << "\n";
            //Push audio buffer to temp vector
            // 0 - 80 (16 azimuths)
            rightVec.push_back(copyR);
            leftVec.push_back(copyL);
            // 180 - 260 (16 azimuths)
            positiveBehindRightVec.push_back(copyRBehind);
            positiveBehindLeftVec.push_back(copyLBehind);

            //disregard the 0th index for negative
            if(i!=0) {
                // 355 - 280 (backwards) (15 azimuths)
                leftVecNeg.push_back(copyLNegative);
                rightVecNeg.push_back(copyRNegative);
                // 175 - 100 (backwards) (15 azimuths)
                negativeBehindLeftVec.push_back(copyLNegativeBehind);
                negativeBehindRightVec.push_back(copyRNegativeBehind);
            }
        }

        //Reorder all Audio buffers to one vector

        for(int i =0; i <  leftVec.size(); i++){
            leftHRIR.push_back(leftVec.at(i));
            rightHRIR.push_back(rightVec.at(i));
        }

        for(int i = negativeBehindRightVec.size() - 1; i >= 0 ; i--){
            leftHRIR.push_back(negativeBehindLeftVec.at(i));
            rightHRIR.push_back(negativeBehindRightVec.at(i));
        }

        for(int i = 0; i < positiveBehindLeftVec.size()  ; i++){
            leftHRIR.push_back(positiveBehindLeftVec.at(i));
            rightHRIR.push_back(positiveBehindRightVec.at(i));
        }

        for(int i = leftVecNeg.size() - 1; i >= 0 ; i--){
            leftHRIR.push_back(leftVecNeg.at(i));
            rightHRIR.push_back(rightVecNeg.at(i));
        }



        std::cout << "\nHRIR Vector<> Size(Right):"<< rightHRIR.size() << "\n";
        std::cout << "HRIR Vector<> Size(Left):"<< leftHRIR.size() << "\n";
    }

    void loadConvolutionFile(){
        AudioSampleBuffer sampleBufferLeft;
        AudioSampleBuffer sampleBufferRight;
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
//        File left0 = File(dir.getChildFile ("Resources").getChildFile("cassette_recorder.wav"));
//        File right0 = File(dir.getChildFile ("Resources").getChildFile("guitar_amp.wav"));
        std::unique_ptr<AudioFormatReader> readerLeft(formatManager1.createReaderFor(left0));
        std::unique_ptr<AudioFormatReader> readerRight(formatManager1.createReaderFor(right0));


        //Load Left HRIR
        if (readerLeft.get() != nullptr) {
            auto duration = readerLeft->lengthInSamples / readerLeft->sampleRate;

            sampleBufferLeft.setSize(readerLeft->numChannels, (int) readerRight->lengthInSamples);
            readerLeft->read(&sampleBufferLeft, 0, (int) readerLeft->lengthInSamples, 0, true, true);
        }

        //Load Right HRIR
        if (readerRight.get() != nullptr) {
            auto duration = readerRight->lengthInSamples / readerRight->sampleRate;

            sampleBufferRight.setSize(readerRight->numChannels, (int) readerRight->lengthInSamples);
            readerRight->read(&sampleBufferRight, 0, (int) readerRight->lengthInSamples, 0, true, true);

        }

        if(readerRight != nullptr) {
            std::cout << "File loaded HRIR Right into audio buffer\n";
            std::cout << "Audio Buffer #Samples:" << sampleBufferRight.getNumSamples() << "\n";
            std::cout << "Audio Buffer #Channels:" << sampleBufferRight.getNumChannels() << "\n";
        }

        if(readerLeft != nullptr) {
            std::cout << "File loaded HRIR Left into audio buffer\n";
            std::cout << "Audio Buffer #Samples:" << sampleBufferLeft.getNumSamples() << "\n";
            std::cout << "Audio Buffer #Channels:" << sampleBufferLeft.getNumChannels() << "\n";
        }


//        //--------------------Reshape the HRIR to the correct shape-------------------------
        rightZero.setSize(1, (int)200);
        int count = 0;
        while(count++ < 199) {
            //Reallocate the buffer to the correct shape
            rightZero.setSample(0, count, sampleBufferRight.getSample(count, 9));
        }

        leftZero.setSize(1, (int)200);
        count = 0;
        while(count++ < 199) {

            //Reallocate the buffer to the correct shape
            leftZero.setSample(0, count, sampleBufferLeft.getSample(count, 9));

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
    //Gui Elements
    TextButton openButton;
    TextButton playButton;
    TextButton stopButton;
    ToggleButton loopingToggle;
    Label currentPositionLabel;

    double lastSampleRate;
    AudioFormatManager formatManager;
    std::unique_ptr<AudioFormatReaderSource> readerSource;
    AudioTransportSource transportSource;
    std::vector<std::unique_ptr<AudioTransportSource>> audioSources;
    //MemoryAudioSource memoryAudioSource;
    TransportState state;

    //This is where the processing of the convolution will occur.
    juce::dsp::ProcessorChain<juce::dsp::Convolution> processorChain;
    enum
    {
        convolutionIndex
    };

    std::unique_ptr<ConProcessorRight> conProcessorRight;
    std::unique_ptr<ConProcessorLeft> conProcessorLeft;

    FilterProcessor filter;
    AudioFormatManager formatManager1;
    AudioSampleBuffer pianoBufferL;
    AudioSampleBuffer pianoBufferR;
    std::unique_ptr<AudioSampleBuffer>  tempL;
    std::unique_ptr<AudioSampleBuffer>  tempR;
    std::unique_ptr<AudioSampleBuffer>  inputL;
    std::unique_ptr<AudioSampleBuffer>  inputR;
    Random random;
    AudioSampleBuffer filterBuffer;
    double sampleRate = 44100.0;
    MidiBuffer emptyMidi;
    int blockSize;
    int samplesExpected;
    std::vector<AudioSampleBuffer> rightVec;
    std::vector<AudioSampleBuffer> leftVec;
    std::vector<AudioSampleBuffer> rightVecNeg;
    std::vector<AudioSampleBuffer> leftVecNeg;
    std::vector<AudioSampleBuffer> leftHRIR;
    std::vector<AudioSampleBuffer> rightHRIR;
    std::vector<AudioSampleBuffer> audioBuffers;
    std::vector<AudioPlayer> audioList;

    RelativeTime relativeTime;
    int impulseIndex = 0;
    int degrees = 0;
    int playhead;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};