/*==============================================================================
//                      3D Basketball Audio
//          An 3d audio simulation of in game sounds and pressure
//==============================================================================
//Todo:--------List of things to do-------------------------
// - Add sound files to be processed. Look at the openButtonClicked function
// - Create the gui for the app and create state variables for the different options
// - Figure out a positioning algorithm for players
//
//
//
//
*/

#pragma once

//Foward Decleration for typedef
struct HRTFData;
class ConProcessorLeft;
class ConProcessorRight;
class AudioPlayer;

struct Node;
//Typedefs for simpler objects
typedef dsp::Matrix<double> Mat;
typedef std::map<int, HRTFData> AzimuthInnerMap;
typedef std::map<int, AzimuthInnerMap> AzimuthMap;
AudioSampleBuffer rightZero;
AudioSampleBuffer leftZero;
//==============================================================================
//                  Helper Functions
//==============================================================================

std::vector<int> azimuthAngles = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 55, 65, 80, 90,
                                  100, 115, 125, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180,
                                  185, 190, 195, 200, 205, 210, 215, 220, 225, 235, 245, 260, 270,
                                  280, 295, 305, 315, 320, 325, 330, 335, 340, 345, 350, 355
};

float decibelsToGain(float decibels){
    return  pow(10.0,decibels/20);
}

float gainToDecibels(float gain){
    return  20.0 * log10(gain);
}

struct Position{
    float x;
    float y;
    Position(): x(0), y(0){}
    Position(float x, float y): x(x), y(y){}
};

struct PositionSpherical{
    float azimuth;
    float elevation;
    float radius;

    PositionSpherical(): azimuth(0), elevation(0), radius(1){}
    PositionSpherical(float az, float el, float rad): azimuth(az), elevation(el), radius(rad){}
};

Position sphereToVector(double radius, double azimuth) {
    double x = cos(degreesToRadians(azimuth));
    double y = radius * sin(degreesToRadians(azimuth));

    Position temp;
    temp.x = x;
    temp.y = y;
    return temp;
}

float radianToDegrees(float num){
    if (num * 180.0/3.141592653589793238463 < 0)
        return num * 180.0/3.141592653589793238463 + 360;
    if (num * 180.0/3.141592653589793238463 > 360)
        return num * 180.0/3.141592653589793238463 - 360;
    return num * 180.0/3.141592653589793238463;
}

PositionSpherical vectorToSphere(Position pos){
    PositionSpherical posSphere;
    posSphere.radius = sqrt(pow(pos.x,2) + pow(pos.y,2));
    posSphere.azimuth = atan(pos.x/pos.y);
    posSphere.azimuth =radianToDegrees(posSphere.azimuth);
    posSphere.elevation = 0;

    return posSphere;
}

struct Node{
    std::shared_ptr<Node> next;
    Position current;

    Node(Position pos, std::shared_ptr<Node> next): current(pos), next(next){
    }

    Node(Position pos){
        current = pos;
    }

};

int findClosestHRTF(float azimuth){

    int min = 0;
    float minNum = abs(azimuth - azimuthAngles.at(0));
    for(int i =0; i < azimuthAngles.size(); i++){
        if( abs(azimuth - azimuthAngles.at(i)) < minNum)
        {
            min = i;
            minNum = abs(azimuth - azimuthAngles.at(i));
        }
    }
    return min;
}

//==============================================================================
//              Audio Player Object
//              Plays stationary sounds
//==============================================================================

struct AudioPlayer {
    AudioSampleBuffer buffer;
    AudioSampleBuffer convolvedBuffer;
    int playHead;
    int azimuth;
    int elevation;
    float gain;

    AudioPlayer():playHead(0), azimuth(0), elevation(0), gain(0){}

    AudioPlayer(AudioSampleBuffer buffer, float gain) :
            buffer(buffer),
            convolvedBuffer(buffer),
            playHead(0),
            gain(gain) {}

    AudioPlayer(AudioSampleBuffer buffer, int elv, int az) :
            buffer(buffer),
            convolvedBuffer(buffer),
            azimuth(az),
            elevation(elv) {}

    AudioPlayer &operator=(AudioPlayer other) // (1)
    {
        swap(*this, other); // (2)
        return *this;
    }

    friend void swap(AudioPlayer &first, AudioPlayer &second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two objects,
        // the two objects are effectively swapped
        swap(first.buffer, second.buffer);
        swap(first.convolvedBuffer, second.convolvedBuffer);
        swap(first.azimuth, second.azimuth);
        swap(first.elevation, second.elevation);
        swap(first.playHead, second.playHead);
        swap(first.gain, second.gain);
    }
};

//==============================================================================
//              HRTF DATA STRUCT
//
//==============================================================================

struct HRTFData {
    AudioSampleBuffer hrtfL;
    AudioSampleBuffer hrtfR;
    int azimuth;
    int elevation;
    float distance;
    float rmsLeft;
    float rmsRight;
//==============================================================================

    HRTFData()
            : azimuth(0),
              elevation(0),
              distance(0),
              rmsLeft(0),
              rmsRight(0) {}
//==============================================================================

    HRTFData(AudioSampleBuffer l, AudioSampleBuffer r, int a, int e, float dist)
            : hrtfL(l),
              hrtfR(r),
              azimuth(a),
              elevation(e),
              distance(dist),
              rmsLeft(0),
              rmsRight(0) {
        calculateRms();
    }
//==============================================================================

    HRTFData &operator=(HRTFData other) // (1)
    {
        swap(*this, other); // (2)
        return *this;
    }
//==============================================================================

    void update(AudioSampleBuffer l, AudioSampleBuffer r, int az, int elv, int dist) {
        hrtfL = l;
        hrtfR = r;
        azimuth = az;
        elevation = elv;
        distance = dist;
        calculateRms();
    }
//==============================================================================

    friend void swap(HRTFData &first, HRTFData &second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two objects,
        // the two objects are effectively swapped
        swap(first.hrtfL, second.hrtfL);
        swap(first.hrtfR, second.hrtfR);
        swap(first.azimuth, second.azimuth);
        swap(first.elevation, second.elevation);
        swap(first.distance, second.distance);
        swap(first.rmsRight, second.rmsRight);
        swap(first.rmsLeft, second.rmsLeft);

    }
//==============================================================================

    bool operator<(const HRTFData &other) const {
        return azimuth < other.azimuth;
    }
//==============================================================================

    static std::vector<HRTFData> SortByAzimuth(std::vector<HRTFData> data) {
        std::vector<HRTFData> data_copy = data;
        std::sort(data_copy.begin(), data_copy.end());
        return data_copy;
    }
//==============================================================================

    bool operator==(const HRTFData &r) const {
        return r.azimuth == azimuth;
    }
//==============================================================================

    static HRTFData &findByAzimuth(std::vector<HRTFData> &data, int az) {
        std::vector<HRTFData>::iterator it;
        HRTFData temp;
        temp.azimuth = az;
        it = std::find(data.begin(), data.end(), temp);
        if (it != data.end()) {
            std::cout << "Found::" << it->azimuth << " " << std::endl;
            return *it;
        } else {
            std::cout << "Item not Found" << std::endl;
            return *it;
        }
    }
//==============================================================================

    void calculateRms() {
        rmsLeft = hrtfL.getRMSLevel(0, 0, 200);
        rmsRight = hrtfR.getRMSLevel(0, 0, 200);
    }
//==============================================================================

    void printRms() {
        std::cout << " AZ " << azimuth << "\n";
        std::cout << "Right RMS:  " << rmsRight << "\n";
        std::cout << "Left RMS: " << rmsLeft << "\n";
    }
//==============================================================================

    void printAll() {
        std::cout << " Azimuth " << azimuth << "\n";
        std::cout << " Elevation " << elevation << "\n";
        std::cout << " Distance " << distance << "\n";
        std::cout << "Right RMS:  " << rmsRight << "\n";
        std::cout << "Left RMS: " << rmsLeft << "\n";
    }
//==============================================================================

    static void printMap(AzimuthMap mapHrtf) {

        for (auto const &mapInner : mapHrtf) {
            for (auto const &data : mapInner.second) {
                std::cout << "--------------------------------\n";
                std::cout << "Azimuth:" << data.second.azimuth << "\n";
                std::cout << "Elevation:" << data.second.elevation << "\n";
                std::cout << "Distance:" << data.second.distance << "\n";
                std::cout << "Left RMS:" << data.second.rmsLeft << "\n";
                std::cout << "Right RMS:" << data.second.rmsRight << "\n";
            }
        }
    }
};

//==============================================================================
//              Player Position Class
//
//==============================================================================
class Player{
public:
    Position currentPos;
    Position nextPos;
    std::shared_ptr<Node> route;
    std::shared_ptr<Node> routeTail;
    std::shared_ptr<Node> head;
    float speed = 0;
    float magnitude = 0;
    Position direction;
    AudioPlayer audioPlayer;
    HRTFData bufferCurrent;

    int hrtfIndex;
    float gain = 1;
    int steps;


    Player():currentPos(Position()), nextPos(Position()), direction(Position()){
        bufferCurrent.hrtfL = leftZero;
        bufferCurrent.hrtfR = rightZero;
        hrtfIndex = 0;


    }

    Position calculatePosition(float deltaTime){
        Position position;
        currentPos.x = currentPos.x + direction.x * (speed * deltaTime);
        currentPos.y = currentPos.y + direction.y * (speed * deltaTime);

    }

    void calculateDirection(){
        magnitude= sqrt(pow(currentPos.x,2) + pow(currentPos.y,2));
        direction.x = (nextPos.x - currentPos.x) / magnitude;
        direction.y = (nextPos.y - currentPos.y) / magnitude;
    }

    void printPosition(){
        std::cout << "Current Pos:[" << currentPos.x << ","<< currentPos.y <<  "]\n";
        std::cout << "Direction:[" << direction.x << ","<< direction.y <<  "]\n";
    }

    void buildRoute(Position start){
        route =  std::make_shared<Node>(start, route);
        head = route;
        routeTail = route;
        steps ++;
    }

    void addToRoute( Position pos){
        std::shared_ptr<Node> temp = std::make_shared<Node>(pos);
        routeTail->next = temp;
        temp->next = route;
        routeTail = temp;
        steps++;
    }

};
//==============================================================================
//                      Processors
//
//==============================================================================
class ProcessorBase : public AudioProcessor {
public:
    //==============================================================================
    ProcessorBase() {}

    ~ProcessorBase() {}

    //==============================================================================
    void prepareToPlay(double, int) override {}

    void releaseResources() override {}

    void processBlock(AudioSampleBuffer &, MidiBuffer &) override {}

    //==============================================================================
    AudioProcessorEditor *createEditor() override { return nullptr; }

    bool hasEditor() const override { return false; }

    //==============================================================================
    const String getName() const override { return {}; }

    bool acceptsMidi() const override { return false; }

    bool producesMidi() const override { return false; }

    double getTailLengthSeconds() const override { return 0; }

    //==============================================================================
    int getNumPrograms() override { return 0; }

    int getCurrentProgram() override { return 0; }

    void setCurrentProgram(int) override {}

    const String getProgramName(int) override { return {}; }

    void changeProgramName(int, const String &) override {}

    //==============================================================================
    void getStateInformation(MemoryBlock &) override {}

    void setStateInformation(const void *, int) override {}

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessorBase)
};

//==============================================================================
//                      Low Pass Filter
//
//==============================================================================

class FilterProcessor : public ProcessorBase {

public:
    FilterProcessor() {}

    /*=================================================================================*/

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        *filter.state = *dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 2000.0f);

        dsp::ProcessSpec spec{sampleRate, static_cast<uint32> (samplesPerBlock), 2};
        filter.prepare(spec);
    }

    /*=================================================================================*/

    void processBlock(AudioSampleBuffer &buffer, MidiBuffer &) override {
        dsp::AudioBlock<float> block(buffer);
        dsp::ProcessContextReplacing<float> context(block);
        filter.process(context);
    }

    /*=================================================================================*/

    void reset() override {
        filter.reset();
    }

    /*=================================================================================*/

    const String getName() const override { return "Filter"; }

private:
    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> filter;
};

//==============================================================================
//                     Convolution Processor
//
//==============================================================================

class ConvolutionProcessor : public ProcessorBase {

public:
    ConvolutionProcessor() {
    }

    /*=================================================================================*/

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {

        auto maxSize = static_cast<size_t> (roundToInt (sampleRate * (8192.0 / 44100.0)));
        convolutionLeft.copyAndLoadImpulseResponseFromBuffer(irBufferLeft, 44100, true, true, false,
                                                             maxSize);

        convolutionRight.copyAndLoadImpulseResponseFromBuffer(irBufferRight, 44100, true, true, false,
                                                              maxSize);
        dsp::ProcessSpec spec{sampleRate, static_cast<uint32> (samplesPerBlock), 2};
        convolutionLeft.prepare(spec);
        convolutionRight.prepare(spec);

        rightChannel.setSize(1,samplesPerBlock);
        leftChannel.setSize(1,samplesPerBlock);

    }

    /*=================================================================================*/

    void processBlock(AudioSampleBuffer &buffer, MidiBuffer &) override {
        rightChannel.setSize(1,buffer.getNumSamples());
        leftChannel.setSize(1,buffer.getNumSamples());
        leftChannel.copyFrom(0,0,buffer,0,0,buffer.getNumSamples());
        rightChannel.copyFrom(0,0,buffer,1,0,buffer.getNumSamples());

        dsp::AudioBlock<float> blockLeft(leftChannel);
        dsp::AudioBlock<float> blockRight(rightChannel);
        dsp::ProcessContextReplacing<float> contextLeft(blockLeft);
        dsp::ProcessContextReplacing<float> contextRight(blockRight);

        convolutionLeft.process(contextLeft);
        convolutionRight.process(contextRight);
        buffer.copyFrom(0,0,blockLeft.getChannelPointer(0),leftChannel.getNumSamples());
        buffer.copyFrom(1,0,blockRight.getChannelPointer(0),rightChannel.getNumSamples());

    }

    /*=================================================================================*/

    void reset() override {
        convolutionLeft.reset();
        convolutionRight.reset();
    }

    /*=================================================================================*/

    const String getName() const override { return "Convolution"; }

    AudioSampleBuffer irBufferLeft;
    AudioSampleBuffer irBufferRight;

    AudioSampleBuffer leftChannel;
    AudioSampleBuffer rightChannel;

private:
    juce::dsp::Convolution convolutionLeft;
    juce::dsp::Convolution convolutionRight;
    MemoryBlock irMem;


};
//==============================================================================
//                      Main Application
//
//
//==============================================================================
class MainContentComponent : public AudioAppComponent,
                             public ChangeListener,
                             public Timer {
public:
    MainContentComponent()
            : state(Stopped) {

        setSize (600, 400);

        addAndMakeVisible(frequencySlider);
        frequencySlider.setRange(100, 5000, 100);
        //frequencySlider.setTextValueSuffix(" Hz");

        addAndMakeVisible(azimuthSlider);
        azimuthSlider.setRange(0.0f, 355.0f, 5.0f);
        addAndMakeVisible(azimuthPosition);
        azimuthPosition.setText("azimuthAngle", dontSendNotification);
        azimuthPosition.attachToComponent(&azimuthSlider, true);


        addAndMakeVisible(frequencyLabel);
        frequencyLabel.setText("Crowd size", dontSendNotification);
        frequencyLabel.attachToComponent(&frequencySlider, true);

        addAndMakeVisible(durationSlider);
        durationSlider.setRange(1, 10, 1);
        //durationSlider.setTextValueSuffix(" seconds");

        addAndMakeVisible(durationLabel);
        durationLabel.setText("Number of players", dontSendNotification);
        durationLabel.attachToComponent(&durationSlider, true);

//        addAndMakeVisible(homeButton);
//        homeButton.setClickingTogglesState(true);
//        homeLabel.setText("Home", dontSendNotification);
//        homeLabel.attachToComponent(&homeButton, true);
//
//        addAndMakeVisible(awayButton);
//        awayLabel.setText("Away", dontSendNotification);
//        awayLabel.attachToComponent(&awayButton, true);



        addAndMakeVisible(&playButton);
        playButton.setButtonText("Play");
        playButton.onClick = [this] { playButtonClicked(); };
        playButton.setColour(TextButton::buttonColourId, Colours::green);
        playButton.setEnabled(true);

        addAndMakeVisible(&stopButton);
        stopButton.setButtonText("Stop");
        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setColour(TextButton::buttonColourId, Colours::red);
        stopButton.setEnabled(false);

//        addAndMakeVisible(&loopingToggle);
//        loopingToggle.setButtonText("Loop");
//        loopingToggle.onClick = [this] { loopButtonChanged(); };
//        loopingToggle.setEnabled(true);

        addAndMakeVisible(&currentPositionLabel);
        currentPositionLabel.setText("Stopped", dontSendNotification);

        formatManager.registerBasicFormats();
        formatManager1.registerBasicFormats();

        transportSource = std::unique_ptr<AudioTransportSource>(new AudioTransportSource());
        transportSource.get()->addChangeListener(this);

        setAudioChannels(0, 2);
        startTimer(20);

    }

    /*=================================================================================*/

    ~MainContentComponent() {
        shutdownAudio();
    }

    /*=================================================================================*/

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        samplesExpected = samplesPerBlockExpected;
        relativeTime1 = relativeTime1.milliseconds(0);

        //Set up of HRTF
        loadFileToTransport();
        impulseProcessing();

        loadPlayer("PlayerLoopMono.wav",one);

        //Extra Buffers
        inputL = std::make_unique<AudioSampleBuffer>();
        inputR = std::make_unique<AudioSampleBuffer>();

        //Convolvers
        convolutionProcessor = std::make_unique<ConvolutionProcessor>();

        convolutionProcessor->irBufferLeft = zeroPlane.at(0).hrtfL;
        convolutionProcessor->irBufferRight = zeroPlane.at(0).hrtfR;
        convolutionProcessor->prepareToPlay(sampleRate, samplesPerBlockExpected);

        //-----------Effects chaing prepare to play-----------------
        filter.prepareToPlay(sampleRate, samplesPerBlockExpected);

        //----------Add sounds to the Audio List-----------------------
        //-------------Place Static Sounds here---------------
        loadAudioFile("CrowdDrumLoop.wav", 0.05f);
        placeSound(65, audioList.at(0).buffer);
        loadAudioFile("CrowdMediumClapping.wav", 0.05f);
        placeSound(30, audioList.at(1).buffer);

        std::cout << "prepare to play called\n";
    }
/*=====================Main Buffer Loop============================================*/
    //Buffer to fill
    void getNextAudioBlock(const AudioSourceChannelInfo &bufferToFill) override {

        if (true){

            if (readerSource.get() == nullptr) {
                bufferToFill.clearActiveBufferRegion();
                return;
            }

            if (state == Stopped){
                bufferToFill.buffer->clear();
            }
            //----Add Dynamic Sound Here-------------------
            if (state == Playing) {
//            addAudioBuffers(&bufferToFill, players.at(0).audioPlayer);
//            applyConvolutionSlider(&bufferToFill);

                followRoute(players.at(0));
                addAudioBuffers(&bufferToFill, players.at(0).audioPlayer);
                applyConvolutionPlayer(&bufferToFill,players.at(0));
            }

            //----Add Static Sound -------------------
            if (state == Playing) {
                if (frequencySlider.getValue() > 100) {
                    addAudioBuffers(&bufferToFill, audioList.at(0));
                    addAudioBuffers(&bufferToFill, audioList.at(1));
                }
                if (frequencySlider.getValue() > 1000) {
                }
                if (frequencySlider.getValue() > 3000) {
                    //addAudioBuffers(&bufferToFill, audioList.at(2));
                }
                if (durationSlider.getValue() > 1){

                }
                if (durationSlider.getValue() > 2){

                }

            }

        }

    }

    /*=================================================================================*/
    void addAudioBuffers(const AudioSourceChannelInfo *source, AudioPlayer &toAdd) {
        inputL->setSize(1, source->numSamples);
        inputR->setSize(1, source->numSamples);

        for (int i = 0; i < source->numSamples; ++i) {
            inputL->setSample(0, i, source->buffer->getSample(0, i) + toAdd.buffer.getSample(0, toAdd.playHead));
            inputR->setSample(0, i, source->buffer->getSample(1, i) + toAdd.buffer.getSample(1, toAdd.playHead++));
            toAdd.playHead %= toAdd.buffer.getNumSamples();
        }

        //Reset input buffer to original state
        float *buffLeft = source->buffer->getWritePointer(0, source->startSample);
        for (int i = 0; i < source->numSamples; ++i) {
            buffLeft[i] = inputL->getSample(0, i);
            //buffLeft[i] = 0;
        }

        float *buffRight = source->buffer->getWritePointer(1, source->startSample);
        for (int i = 0; i < source->numSamples; ++i) {
            buffRight[i] = inputR->getSample(0, i);
            //buffRight[i] = 0;
        }
    }

/*=================================================================================*/
    void applyGain(const AudioSourceChannelInfo *buffer, double gain){
        for (int i = 0; i < buffer->buffer->getNumSamples(); ++i) {
            buffer->buffer->setSample(0, i, buffer->buffer->getSample(0, i) * gain);
            buffer->buffer->setSample(1, i, buffer->buffer->getSample(1, i) * gain);
            //std::cout<< "LeftChannel sample:" << leftChannel.getSample(0,i) << "\n";
        }
    }

    /*=================================================================================*/
    AudioSampleBuffer addSilence(AudioSampleBuffer &buffer, double time){
        AudioSampleBuffer temp;
        temp.setSize(2,buffer.getNumSamples() + sampleRate * time);
        float start = sampleRate * time;
        temp.clear();
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            temp.setSample(0,start + i, buffer.getSample(0,i));
            temp.setSample(1,start + i, buffer.getSample(0,i));
        }
        return temp;
    }

    /*=================================================================================*/

    AudioSampleBuffer placeSound(int index, AudioSampleBuffer &inputBuffer) {
        convolutionProcessor->irBufferLeft = zeroPlane.at(index).hrtfL;
        convolutionProcessor->irBufferRight = zeroPlane.at(index).hrtfR;

        convolutionProcessor->prepareToPlay(sampleRate, inputBuffer.getNumSamples());
        convolutionProcessor->processBlock(inputBuffer,emptyMidi);
        return inputBuffer;
    }

    /*=================================================================================*/

    void applyConvolutionSlider(const AudioSourceChannelInfo *buffer) {
        //Get a preprocessed verstion stored called inputL and inputR

        if (state == Stopped)
            return;

        convolutionProcessor->processBlock(*buffer->buffer,emptyMidi);
        (relativeTime += relativeTime.milliseconds(10)).inMilliseconds();
        if (azimuthSlider.getValue()/5 != lastAzimuthPos) {
            std::cout << "Approximate Azimuth Angle: " << zeroPlane.at(azimuthSlider.getValue()/5).azimuth << "\n";
            degrees += 5;
            relativeTime = relativeTime.milliseconds(0);


            //Write new hrir for convolution at new angle
            float *irWriteLeft = convolutionProcessor->irBufferLeft.getWritePointer(0);
            float *irWriteRight = convolutionProcessor->irBufferRight.getWritePointer(0);
            for (int i = 0; i < 200; i++) {
                irWriteLeft[i] = zeroPlane.at(azimuthSlider.getValue() / 5).hrtfL.getSample(0, i);
                irWriteRight[i] = zeroPlane.at(azimuthSlider.getValue() / 5).hrtfR.getSample(0, i);
            }
            impulseIndex++;

            //Reset angle to 0
            if (impulseIndex > zeroPlane.size() - 1) {
                impulseIndex = 0;
                degrees = 0;
            }

            //prepare convolution processors
            convolutionProcessor->prepareToPlay(sampleRate, samplesExpected);
            convolutionProcessor->prepareToPlay(sampleRate, samplesExpected);
            lastAzimuthPos = azimuthSlider.getValue() / 5;
//        }
        }
    }

    /*=================================================================================*/

    void applyConvolutionPlayer(const AudioSourceChannelInfo *buffer, Player& player) {

        //Get a preprocessed verstion stored called inputL and inputR

        if (state == Stopped)
            return;

        convolutionProcessor->processBlock(*buffer->buffer,emptyMidi);

        if (player.hrtfIndex != indexPast) {
            //Reset convolution processes

            //Write new hrir for convolution at new angle
            float *irWriteLeft = convolutionProcessor->irBufferLeft.getWritePointer(0);
            float *irWriteRight = convolutionProcessor->irBufferRight.getWritePointer(0);
            for (int i = 0; i < 200; i++) {
                irWriteLeft[i] = player.bufferCurrent.hrtfL.getSample(0, i);
                irWriteRight[i] = player.bufferCurrent.hrtfR.getSample(0, i);
            }

            //prepare convolution processors
            convolutionProcessor->prepareToPlay(sampleRate, samplesExpected);
            indexPast =player.hrtfIndex;
            std::cout << "convolution reset" << std::endl;
        }
        applyGain(buffer, player.gain);
    }

    /*=================================================================================*/
    void releaseResources() override {
        transportSource->releaseResources();
        convolutionProcessor->releaseResources();
        filter.releaseResources();
    }
    /*=================================================================================*/

    void resized() override {
        const int border = 120;
        openButton.setBounds(border, 10, getWidth() - 20, 20);
        playButton.setBounds(border - 60, 40, getWidth() - 100, 20);
        stopButton.setBounds(border -60 , 70, getWidth() - 100, 20);
        loopingToggle.setBounds(border, 100, getWidth() - 20, 20);
        currentPositionLabel.setBounds(border, 130, getWidth() - 20, 20);

        frequencySlider.setBounds(border,130 + 20, getWidth() - border, 20);
        durationSlider.setBounds(border, 130 + 50, getWidth() - border, 50);
        homeButton.setBounds(border, 130 + 110, 22, 22);
        awayButton.setBounds(border + 100, 130 + 110, 22, 22);
        azimuthPosition.setBounds(border, 130 + 170, getWidth() - border, 50);
        azimuthSlider.setBounds(border, 130 + 200, getWidth() - border, 50);
    }

    /*=================================================================================*/

    void changeListenerCallback(ChangeBroadcaster *source) override {
        if (source == transportSource.get()) {
            if (transportSource->isPlaying())
                changeState(Playing);
            else
                changeState(Stopped);
        }
    }

    /*=================================================================================*/

    void timerCallback() override {
        if (transportSource->isPlaying()) {

            position.operator+=(position.milliseconds(18));
            auto minutes = ((int) position.inMinutes()) % 60;
            auto seconds = ((int) position.inSeconds()) % 60;
            auto millis = ((int) position.inMilliseconds()) % 1000;
            auto positionString = String::formatted("%02d:%02d:%03d", minutes, seconds, millis);

            currentPositionLabel.setText(positionString, dontSendNotification);
        } else {
            currentPositionLabel.setText("Stopped", dontSendNotification);
            position= position.milliseconds(0);
        }
    }

    /*=================================================================================*/

    void updateLoopState(bool shouldLoop) {
        if (readerSource.get() != nullptr)
            readerSource->setLooping(shouldLoop);
        if (readerSource1.get() != nullptr)
            readerSource1->setLooping(shouldLoop);
    }
    /*=================================================================================*/

private:
    enum TransportState {
        Stopped,
        Starting,
        Playing,
        Stopping
    };

    void changeState(TransportState newState) {
        if (state != newState) {
            state = newState;

            switch (state) {
                case Stopped:
                    stopButton.setEnabled(false);
                    playButton.setEnabled(true);
                    transportSource->setPosition(0.0);
                    break;

                case Starting:
                    playButton.setEnabled(false);
                    transportSource->start();
                    break;

                case Playing:
                    stopButton.setEnabled(true);
                    break;

                case Stopping:
                    transportSource->stop();
                    break;
            }
        }
    }

    /*=================================================================================*/

    void loadFileToTransport() {
        formatManager.getDefaultFormat();
        formatManager1.getDefaultFormat();
        AudioSampleBuffer channel1;
        auto dir = File::getCurrentWorkingDirectory();
        int numTries = 0;

        //find the resources dir
        while (!dir.getChildFile("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        File temp = File(dir.getChildFile("Resources").getChildFile("PlayerLoopMono.wav"));
        File temp1 = File(dir.getChildFile("Resources").getChildFile("Register.wav"));
        auto *reader = formatManager.createReaderFor(temp);


        formatManager1.createReaderFor(temp1);

        if (reader != nullptr) {
            std::unique_ptr<AudioFormatReaderSource> newSource(new AudioFormatReaderSource(reader, true));
            transportSource->setSource(newSource.get(), 0, nullptr, reader->sampleRate);
            playButton.setEnabled(true);
            readerSource.reset(newSource.release());
            transportSource.get()->setGain(2.0);
            std::cout << "AudioFile Loaded! \n";
        }
    }

    /*=================================================================================*/

    void loadAudioFile(String fileName, float gain) {
        auto dir = File::getCurrentWorkingDirectory();
        int numTries = 0;
        AudioSampleBuffer sampleBuffer;
        AudioFormatManager formatMan;
        formatMan.registerBasicFormats();
        formatMan.getDefaultFormat();

        //find the resources dir
        while (!dir.getChildFile("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        File temp = File(dir.getChildFile("Resources").getChildFile(fileName));
        while (!dir.getChildFile("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        std::unique_ptr<AudioFormatReader> source(formatManager.createReaderFor(temp));

        if (source.get() != nullptr) {
            sampleBuffer.setSize(2, (int) source->lengthInSamples);
            source->read(&sampleBuffer, 0, (int) source->lengthInSamples, 0, true, true);
        }

        for (int i = 0; i < sampleBuffer.getNumSamples(); ++i) {
            sampleBuffer.setSample(0, i, sampleBuffer.getSample(0, i) * gain);
            sampleBuffer.setSample(1, i, sampleBuffer.getSample(1, i) * gain);
            //std::cout<< "LeftChannel sample:" << leftChannel.getSample(0,i) << "\n";
        }
        AudioPlayer tempAudio(sampleBuffer, gain);
        audioList.push_back(tempAudio);

    }

    AudioSampleBuffer loadAudioFileToBuffer(String fileName, float gain) {
        auto dir = File::getCurrentWorkingDirectory();
        int numTries = 0;
        AudioSampleBuffer sampleBuffer;
        AudioFormatManager formatMan;
        formatMan.registerBasicFormats();
        formatMan.getDefaultFormat();

        //find the resources dir
        while (!dir.getChildFile("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        File temp = File(dir.getChildFile("Resources").getChildFile(fileName));
        while (!dir.getChildFile("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        std::unique_ptr<AudioFormatReader> source(formatManager.createReaderFor(temp));

        if (source.get() != nullptr) {
            sampleBuffer.setSize(2, (int) source->lengthInSamples);
            source->read(&sampleBuffer, 0, (int) source->lengthInSamples, 0, true, true);
        }

        for (int i = 0; i < sampleBuffer.getNumSamples(); ++i) {
            sampleBuffer.setSample(0, i, sampleBuffer.getSample(0, i) * gain);
            sampleBuffer.setSample(1, i, sampleBuffer.getSample(1, i) * gain);
            //std::cout<< "LeftChannel sample:" << leftChannel.getSample(0,i) << "\n";
        }

        return sampleBuffer;

    }
    /*=================================================================================*/
    AudioPlayer loadAudioFilePlayer(String fileName, float gain) {
        auto dir = File::getCurrentWorkingDirectory();
        int numTries = 0;
        AudioSampleBuffer sampleBuffer;
        AudioFormatManager formatMan;
        formatMan.registerBasicFormats();
        formatMan.getDefaultFormat();

        //find the resources dir
        while (!dir.getChildFile("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        File temp = File(dir.getChildFile("Resources").getChildFile(fileName));
        while (!dir.getChildFile("Resources").exists() && numTries++ < 15) {
            dir = dir.getParentDirectory();
        }

        std::unique_ptr<AudioFormatReader> source(formatManager.createReaderFor(temp));

        if (source.get() != nullptr) {
            sampleBuffer.setSize(2, (int) source->lengthInSamples);
            source->read(&sampleBuffer, 0, (int) source->lengthInSamples, 0, true, true);
        }

        for (int i = 0; i < sampleBuffer.getNumSamples(); ++i) {
            sampleBuffer.setSample(0, i, sampleBuffer.getSample(0, i) * gain);
            sampleBuffer.setSample(1, i, sampleBuffer.getSample(1, i) * gain);
            //std::cout<< "LeftChannel sample:" << leftChannel.getSample(0,i) << "\n";
        }
        AudioPlayer tempAudio(sampleBuffer, gain);
        return tempAudio;

    }
    /*=================================================================================*/

    void playButtonClicked() {
        updateLoopState(loopingToggle.getToggleState());
        changeState(Starting);
    }

    /*=================================================================================*/

    void stopButtonClicked() {
        changeState(Stopping);
    }

    /*=================================================================================*/

    void loopButtonChanged() {
        updateLoopState(loopingToggle.getToggleState());
    }

    /*=================================================================================*/

    void impulseProcessing() {
        loadConvolutionFiles();
    }

    /*=================================================================================*/

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

        AudioSampleBuffer posFrontPlusSixL;
        AudioSampleBuffer posFrontPlusSixR;

        AudioSampleBuffer posBehindPlusSixL;
        AudioSampleBuffer posBehindPlusSixR;

        AudioSampleBuffer negFrontPlusSixL;
        AudioSampleBuffer negFrontPlusSixR;

        AudioSampleBuffer negBehindPlusSixL;
        AudioSampleBuffer negBehindPlusSixR;

        AudioSampleBuffer posFrontMinusSixL;
        AudioSampleBuffer posFrontMinusSixR;

        AudioSampleBuffer posBehindMinusSixL;
        AudioSampleBuffer posBehindMinusSixR;

        AudioSampleBuffer negFrontMinusSixL;
        AudioSampleBuffer negFrontMinusSixR;

        AudioSampleBuffer negBehindMinusSixL;
        AudioSampleBuffer negBehindMinusSixR;

        //temp vectors
        std::vector<AudioSampleBuffer> positiveBehindRightVec;
        std::vector<AudioSampleBuffer> positiveBehindLeftVec;

        std::vector<AudioSampleBuffer> negativeBehindRightVec;
        std::vector<AudioSampleBuffer> negativeBehindLeftVec;

        AudioFormatManager formatManager1;
        formatManager1.registerBasicFormats();
        formatManager1.getDefaultFormat();
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
            if (i != 0) {
                String negR = "neg";
                negR += i;
                negR += "azright.wav";
                String negL = "neg";
                negL += i;
                negL += "azleft.wav";
                File negLeft = File(dir.getChildFile("Resources").getChildFile("subject48").getChildFile(negL));
                File negRight = File(dir.getChildFile("Resources").getChildFile("subject48").getChildFile(negR));
                std::unique_ptr<AudioFormatReader> readerNegLeft(formatManager1.createReaderFor(negLeft));
                std::unique_ptr<AudioFormatReader> readerNegRight(formatManager1.createReaderFor(negRight));
                //-----------------Negative Azimuth readers-----------------------------------
                //Load Left HRIR
                if (readerNegLeft.get() != nullptr) {
                    sampleBufferLeftNeg.setSize(readerNegLeft->numChannels, (int) readerNegLeft->lengthInSamples);
                    readerNegLeft->read(&sampleBufferLeftNeg, 0, (int) readerNegLeft->lengthInSamples, 0, true, true);
                }
                //Load Right HRIR
                if (readerNegRight.get() != nullptr) {
                    sampleBufferRightNeg.setSize(readerNegRight->numChannels, (int) readerNegRight->lengthInSamples);
                    readerNegRight->read(&sampleBufferRightNeg, 0, (int) readerNegRight->lengthInSamples, 0, true,
                                         true);
                }
            }
            File fileLeft = File(dir.getChildFile("Resources").getChildFile("subject48").getChildFile(fileL));
            File fileRight = File(dir.getChildFile("Resources").getChildFile("subject48").getChildFile(fileR));

            std::unique_ptr<AudioFormatReader> readerLeft(formatManager1.createReaderFor(fileLeft));
            std::unique_ptr<AudioFormatReader> readerRight(formatManager1.createReaderFor(fileRight));


            //Load Left HRIR
            if (readerLeft.get() != nullptr) {
                sampleBufferLeft.setSize(readerLeft->numChannels, (int) readerRight->lengthInSamples);
                readerLeft->read(&sampleBufferLeft, 0, (int) readerLeft->lengthInSamples, 0, true, true);
            }
            //Load Right HRIR
            if (readerRight.get() != nullptr) {
                sampleBufferRight.setSize(readerRight->numChannels, (int) readerRight->lengthInSamples);
                readerRight->read(&sampleBufferRight, 0, (int) readerRight->lengthInSamples, 0, true, true);
            }

            //--------------------Reshape the HRIR to the correct shape-------------------------
            copyR.setSize(1, 200);          //front +
            copyRNegative.setSize(1, 200);   //front -
            copyRBehind.setSize(1, 200);         //behind +
            copyRNegativeBehind.setSize(1, 200); //behind -
            posFrontPlusSixL.setSize(1, 200);
            posFrontPlusSixR.setSize(1, 200);
            posFrontMinusSixL.setSize(1, 200);
            posFrontMinusSixR.setSize(1, 200);
            posBehindMinusSixL.setSize(1, 200);
            posBehindMinusSixR.setSize(1, 200);
            posBehindPlusSixL.setSize(1, 200);
            posBehindPlusSixR.setSize(1, 200);
            negFrontMinusSixL.setSize(1, 200);
            negFrontMinusSixR.setSize(1, 200);
            negFrontPlusSixL.setSize(1, 200);
            negFrontPlusSixR.setSize(1, 200);
            negBehindMinusSixL.setSize(1, 200);
            negBehindMinusSixR.setSize(1, 200);
            negBehindPlusSixL.setSize(1, 200);
            negBehindPlusSixR.setSize(1, 200);

            //Front level elevation 9
            //Back level elevation 39
            const int elevationFront = 8;
            const int elevationBehind = 40;
            const int elevationFrontMinusSix = 7;
            const int elevationFrontPlusSix = 9;
            const int elevationBehindMinusSix = 41;
            const int elevationBehindPlusSix = 39;
            int count = 0;
            while (count++ < 199) {

                //-----------------------Right Allocation---------------------------------------

                //Reallocate the buffer to the correct shape
                copyR.setSample(0, count, sampleBufferRight.getSample(count, elevationFront));
                copyRBehind.setSample(0, count, sampleBufferRight.getSample(count, elevationBehind));

                posFrontMinusSixR.setSample(0, count, sampleBufferRight.getSample(count, elevationFrontMinusSix));
                posFrontPlusSixR.setSample(0, count, sampleBufferRight.getSample(count, elevationFrontPlusSix));
                posBehindMinusSixR.setSample(0, count, sampleBufferRight.getSample(count, elevationBehindMinusSix));
                posBehindPlusSixR.setSample(0, count, sampleBufferRight.getSample(count, elevationBehindPlusSix));

                if (i != 0) {
                    copyRNegative.setSample(0, count, sampleBufferRightNeg.getSample(count, elevationFront));
                    copyRNegativeBehind.setSample(0, count, sampleBufferRightNeg.getSample(count, elevationBehind));

                    negFrontMinusSixR.setSample(0, count, sampleBufferRightNeg.getSample(count, elevationFrontMinusSix));
                    negFrontPlusSixR.setSample(0, count, sampleBufferRightNeg.getSample(count, elevationFrontPlusSix));
                    negBehindMinusSixR.setSample(0, count, sampleBufferRightNeg.getSample(count, elevationBehindMinusSix));
                    negBehindPlusSixR.setSample(0, count, sampleBufferRightNeg.getSample(count, elevationBehindPlusSix));
                }
            }

            //-----------------------Left Allocation---------------------------------------
            copyL.setSize(1, (int) 200);         //front +
            copyLNegative.setSize(1, 200);       //front -
            copyLBehind.setSize(1, 200);         //behind +
            copyLNegativeBehind.setSize(1, 200); //behind -
            count = 0;
            while (count++ < 199) {
                //Reallocate the buffer to the correct shape
                copyL.setSample(0, count, sampleBufferLeft.getSample(count, elevationFront));
                copyLBehind.setSample(0, count, sampleBufferLeft.getSample(count, elevationBehind));
                posFrontMinusSixL.setSample(0, count, sampleBufferLeft.getSample(count, elevationFrontMinusSix));
                posFrontPlusSixL.setSample(0, count, sampleBufferLeft.getSample(count, elevationFrontPlusSix));
                posBehindMinusSixL.setSample(0, count, sampleBufferLeft.getSample(count, elevationBehindMinusSix));
                posBehindPlusSixL.setSample(0, count, sampleBufferLeft.getSample(count, elevationBehindPlusSix));


                //disregard the 0th index for negative
                if (i != 0) {
                    copyLNegative.setSample(0, count, sampleBufferLeftNeg.getSample(count, elevationFront));
                    copyLNegativeBehind.setSample(0, count, sampleBufferLeftNeg.getSample(count, elevationBehind));

                    negFrontMinusSixL.setSample(0, count, sampleBufferLeftNeg.getSample(count, elevationFrontMinusSix));
                    negFrontPlusSixL.setSample(0, count, sampleBufferLeftNeg.getSample(count, elevationFrontPlusSix));
                    negBehindMinusSixL.setSample(0, count, sampleBufferLeftNeg.getSample(count, elevationBehindMinusSix));
                    negBehindPlusSixL.setSample(0, count, sampleBufferLeftNeg.getSample(count, elevationBehindPlusSix));

                }
            }

            //Elevations:
            //[-45 -39 -34 -28 -23 -17 -11 -6  0   6         || 0-9
            //  11  17  23  28  34  39  45 51  56  62        || 10-19
            //  68  73  79  84  90  96 101 107 113 118       || 20-29
            //  124 129 135 141 146 152 158 163 169 174      || 30-39
            //  180 186 191 197 203 208 214 219 225 231]     || 40-49
            //Push audio buffer to temp vector

            if (i <= 45) {
                // 0 - 80 (16 azimuths)
                rightVec.push_back(copyR);
                leftVec.push_back(copyL);
                HRTFData tempFront(copyL, copyR, i, 0, 1);
                zeroPlane.push_back(tempFront);

                // 180 - 260 (16 azimuths)

                positiveBehindRightVec.push_back(copyRBehind);
                positiveBehindLeftVec.push_back(copyLBehind);
                tempFront.update(copyLBehind, copyRBehind, 180 - i, 0, 1);
                zeroPlane.push_back(tempFront);

                //Q1
                HRTFData tempMinus(posFrontMinusSixL, posFrontMinusSixR, i, -6, 1);
                HRTFData tempPlus(posFrontPlusSixL, posFrontPlusSixR, i, 6, 1);

                minusSix.push_back(tempMinus);
                plusSix.push_back(tempPlus);


                //Q2
                HRTFData tempMinusB(posBehindMinusSixL, posBehindMinusSixR, 180 - (i), -6, 1);
                HRTFData tempPlusB(posBehindPlusSixL, posBehindPlusSixR, 180 - (i), 6, 1);

                minusSix.push_back(tempMinusB);
                plusSix.push_back(tempPlusB);

                if (i != 0) {
                    //Q3
                    HRTFData tempMinusNegBehind(negBehindMinusSixL, negBehindMinusSixR, 180 + (i), -6, 1);
                    HRTFData tempPlusNegBehind(negBehindPlusSixL, negBehindPlusSixR, 180 + (i), 6, 1);
                    minusSix.push_back(tempMinusNegBehind);
                    plusSix.push_back(tempPlusNegBehind);

                    //Q4
                    HRTFData tempMinusNeg(negFrontMinusSixL, negFrontMinusSixR, 360 - (i), -6, 1);
                    HRTFData tempPlusNeg(negFrontPlusSixL, negFrontPlusSixR, 360 - (i), 6, 1);

                    minusSix.push_back(tempMinusNeg);
                    plusSix.push_back(tempPlusNeg);
                }
            }


            if (i == 55 || i == 65 || i == 80) {

                // 0-80 (16 azimuths)
                rightVec.push_back(copyR);
                leftVec.push_back(copyL);
                HRTFData tempFront;
                tempFront.update(copyL, copyR, i, 0, 1);
                zeroPlane.push_back(tempFront);

                // 180-80 (16 azimuths)
                positiveBehindRightVec.push_back(copyRBehind);
                positiveBehindLeftVec.push_back(copyLBehind);
                tempFront.update(copyLBehind, copyRBehind, 180 - i, 0, 1);
                zeroPlane.push_back(tempFront);

                //Q1
                HRTFData tempMinus(posFrontMinusSixL, posFrontMinusSixR, i, -6, 1);
                HRTFData tempPlus(posFrontPlusSixL, posFrontPlusSixR, i, 6, 1);

                minusSix.push_back(tempMinus);
                plusSix.push_back(tempPlus);


                //Q2
                HRTFData tempMinusB(posBehindMinusSixL, posBehindMinusSixR, 180 - (i), -6, 1);
                HRTFData tempPlusB(posBehindPlusSixL, posBehindPlusSixR, 180 - (i), 6, 1);

                minusSix.push_back(tempMinusB);
                plusSix.push_back(tempPlusB);

                //Q3
                HRTFData tempMinusNegBehind(negBehindMinusSixL, negBehindMinusSixR, 180 + (i), -6, 1);
                HRTFData tempPlusNegBehind(negBehindPlusSixL, negBehindPlusSixR, 180 + (i), 6, 1);
                minusSix.push_back(tempMinusNegBehind);
                plusSix.push_back(tempPlusNegBehind);

                //Q4
                HRTFData tempMinusNeg(negFrontMinusSixL, negFrontMinusSixR, 360 - (i), -6, 1);
                HRTFData tempPlusNeg(negFrontPlusSixL, negFrontPlusSixR, 360 - (i), 6, 1);

                minusSix.push_back(tempMinusNeg);
                plusSix.push_back(tempPlusNeg);
            }

            //disregard the 0th index for negative
            if (i != 0) {
                if (i <= 45) {
                    // 355 - 280 (backwards) (15 azimuths)
                    leftVecNeg.push_back(copyLNegative);
                    rightVecNeg.push_back(copyRNegative);

                    HRTFData tempFront;
                    tempFront.update(copyLNegative, copyRNegative, 360 - i, 0, 1);
                    zeroPlane.push_back(tempFront);

                    // 185 - 260 (backwards) (15 azimuths)
                    negativeBehindLeftVec.push_back(copyLNegativeBehind);
                    negativeBehindRightVec.push_back(copyRNegativeBehind);
                    tempFront.update(copyLNegativeBehind, copyRNegativeBehind, 180 + i, 0, 1);
                    zeroPlane.push_back(tempFront);
                }
                if (i == 55 || i == 65 || i == 80) {

                    // 355 - 280 (backwards) (15 azimuths)
                    leftVecNeg.push_back(copyLNegative);
                    rightVecNeg.push_back(copyRNegative);
                    HRTFData tempFront;
                    tempFront.update(copyLNegative, copyRNegative, 360 - i, 0, 1);
                    zeroPlane.push_back(tempFront);

                    // 185 - 260 (backwards) (15 azimuths)
                    negativeBehindLeftVec.push_back(copyLNegativeBehind);
                    negativeBehindRightVec.push_back(copyRNegativeBehind);
                    tempFront.update(copyLNegativeBehind, copyRNegativeBehind, 180 + i, 0, 1);
                    zeroPlane.push_back(tempFront);
                }
            }
        }

        //Reorder all Audio buffers to one vector
        for (int i = 0; i < leftVec.size(); i++) {
            leftHRIR.push_back(leftVec.at(i));
            rightHRIR.push_back(rightVec.at(i));
        }


        for (int i = (int) positiveBehindLeftVec.size() - 1; i >= 0; i--) {
            leftHRIR.push_back(positiveBehindLeftVec.at(i));
            rightHRIR.push_back(positiveBehindRightVec.at(i));
        }

        for (int i = 0; i < negativeBehindLeftVec.size(); i++) {
            leftHRIR.push_back(negativeBehindLeftVec.at(i));
            rightHRIR.push_back(negativeBehindRightVec.at(i));
        }

        for (int i = (int) leftVecNeg.size() - 1; i >= 0; i--) {
            leftHRIR.push_back(leftVecNeg.at(i));
            rightHRIR.push_back(rightVecNeg.at(i));
        }

        //Sort All the Loaded HRTFS
        minusSix = HRTFData::SortByAzimuth(minusSix);
        plusSix = HRTFData::SortByAzimuth(plusSix);
        zeroPlane = HRTFData::SortByAzimuth(zeroPlane);

        addInterpolatedPoints();
        convertBaselineHrtf();
        leftZero = zeroPlane.at(0).hrtfL;
        rightZero = zeroPlane.at(0).hrtfR;
    }

    /*=================================================================================*/

    void addInterpolatedPoints() {
        //add 50, 60, 70, 75, 85 90, 95
        HRTFData newPoint;
        HRTFData newPoint1;
        HRTFData newPoint2;

        int azimuthNew;

        //pt1, pt2, pt3, az, elev
        HRTFData pt1;
        HRTFData pt2;
        HRTFData pt3;

        //90
        azimuthNew = 90;
        pt1 = HRTFData::findByAzimuth(minusSix, azimuthNew - 10);
        pt2 = HRTFData::findByAzimuth(plusSix, azimuthNew - 10);
        pt3 = HRTFData::findByAzimuth(zeroPlane, azimuthNew + 10);

        newPoint = interpolatePoint(pt1,pt2,pt3, azimuthNew, 0);
        newPoint1 = interpolatePoint(pt1,pt2,pt3, azimuthNew, +6);
        newPoint2 = interpolatePoint(pt1,pt2,pt3, azimuthNew, -6);
        newPoint.printAll();

        zeroPlane.push_back(newPoint);
        minusSix.push_back(newPoint1);
        plusSix.push_back(newPoint2);

        //270
        azimuthNew = 270;
        pt1 = HRTFData::findByAzimuth(minusSix, azimuthNew - 10);
        pt2 = HRTFData::findByAzimuth(plusSix, azimuthNew - 10);
        pt3 = HRTFData::findByAzimuth(zeroPlane,azimuthNew + 10);

        newPoint = interpolatePoint(pt1,pt2,pt3, azimuthNew, 0);
        newPoint1 = interpolatePoint(pt1,pt2,pt3, azimuthNew, +6);
        newPoint2 = interpolatePoint(pt1,pt2,pt3, azimuthNew, -6);
        zeroPlane.push_back(newPoint);
        minusSix.push_back(newPoint1);
        plusSix.push_back(newPoint2);
        newPoint.printAll();

        minusSix = HRTFData::SortByAzimuth(minusSix);
        plusSix = HRTFData::SortByAzimuth(plusSix);
        zeroPlane = HRTFData::SortByAzimuth(zeroPlane);

        int index = 0;
        std::vector<int>::iterator it;
        it = azimuthAngles.begin();

        while(true){
            //Create a new point and add to azimuth angles
            if (azimuthAngles[index] != index *5){
                azimuthNew = index * 5;
                pt1 = HRTFData::findByAzimuth(minusSix, azimuthAngles[index - 1]);
                pt2 = HRTFData::findByAzimuth(plusSix, azimuthAngles[index - 1]);
                pt3 = HRTFData::findByAzimuth(zeroPlane,azimuthAngles[index]);
                newPoint = interpolatePoint(pt1,pt2,pt3, azimuthNew, 0);
                newPoint1 = interpolatePoint(pt1,pt2,pt3, azimuthNew, +6);
                newPoint2 = interpolatePoint(pt1,pt2,pt3, azimuthNew, -6);

                zeroPlane.push_back(newPoint);
                minusSix.push_back(newPoint1);
                plusSix.push_back(newPoint2);

                azimuthAngles.insert(it, azimuthNew);
                newPoint.printAll();
                index = 0;
                it = azimuthAngles.begin();
                minusSix = HRTFData::SortByAzimuth(minusSix);
                plusSix = HRTFData::SortByAzimuth(plusSix);
                zeroPlane = HRTFData::SortByAzimuth(zeroPlane);
                if (azimuthNew == 310){break;}
                continue;
            }
            index++;
            it++;
        }
        std::cout << "Finished interpolation \n";
    }

    /*=================================================================================*/

    void convertBaselineHrtf() {
        std::map<int, HRTFData> tempPair;

        for (int i = 0; i < azimuthAngles.size(); i++) {
            HRTFData temp(zeroPlane.at(i).hrtfL, zeroPlane.at(i).hrtfR, azimuthAngles.at(i), 0, 1);
            tempPair.insert(std::pair<int, HRTFData>(azimuthAngles.at(i), temp));
        }
        hrtfMap.insert(std::pair<int, AzimuthInnerMap>(0, tempPair));

        std::cout << "Count Map Size:" << hrtfMap.size();
        HRTFData::printMap(hrtfMap);
    }

    /*=================================================================================*/


    void applyGainToHRTF(HRTFData &pt1, HRTFData &pt2, HRTFData &pt3, Mat gain, int az, int elv, HRTFData &hrtfData) {

        AudioSampleBuffer bufferL;
        AudioSampleBuffer bufferR;
        bufferL.setSize(1, 200);
        bufferR.setSize(1, 200);
        float l;
        float r;

        for (int i = 0; i < 200; i++) {
            l = (gain(0, 0) * pt1.hrtfL.getSample(0, i)) + (gain(0, 1) * pt2.hrtfL.getSample(0, i)) +
                (gain(0, 2) * pt3.hrtfL.getSample(0, i));
            bufferL.setSample(0, i, l);
            r = (gain(0, 0) * pt1.hrtfR.getSample(0, i)) + (gain(0, 1) * pt2.hrtfR.getSample(0, i)) +
                (gain(0, 2) * pt3.hrtfR.getSample(0, i));
            bufferR.setSample(0, i, r);
        }
        HRTFData tempH(bufferL, bufferR, az, elv, 1);
        tempH.calculateRms();
        tempH.printRms();
        hrtfData = tempH;
    }

    /*=================================================================================*/

    HRTFData interpolatePoint(HRTFData pt1, HRTFData pt2, HRTFData pt3, int az, int elv) {

        dsp::Matrix<double> i(1, 3);
        dsp::Matrix<double> j(1, 3);
        dsp::Matrix<double> k(1, 3);
        dsp::Matrix<double> m(1, 3);

        i = getVector(pt1.distance, pt1.azimuth, pt1.elevation + 90);
        j = getVector(pt2.distance, pt2.azimuth, pt2.elevation + 90);
        k = getVector(pt3.distance, pt3.azimuth, pt3.elevation + 90);
        m = getVector(1, az, elv + 90);

        Mat gain(0, 3);
        gain = matrixGains(i, j, k, m);
        HRTFData tempH;
        applyGainToHRTF(pt1, pt2, pt3, gain, az, elv, tempH);

        return tempH;
    }
    /*=================================================================================*/

    //Verified and correct
    dsp::Matrix<double> getVector(double radius, double azimuth, double elevation) {
        double x = cos(degreesToRadians(azimuth)) * sin(degreesToRadians(elevation));
        double y = radius * sin(degreesToRadians(azimuth)) * sin(degreesToRadians(elevation));
        double z = radius * cos(degreesToRadians(elevation));

        std::cout << " x:" << x << " y:" << y << " z:" << z << "\n";
        dsp::Matrix<double> cart(1, 3);
        cart(0, 0) = x;
        cart(0, 1) = y;
        cart(0, 2) = z;

        return cart;
    }

    /*=================================================================================*/

    //This is working and verified
    Mat matrixGains(dsp::Matrix<double> a, dsp::Matrix<double> b, dsp::Matrix<double> c, dsp::Matrix<double> p) {
        dsp::Matrix<double> m(3, 3);
        dsp::Matrix<double> d(1, 3);
        dsp::Matrix<double> minv(3, 3);

        //Vectors
        dsp::Matrix<double> i(1, 3);
        dsp::Matrix<double> j(1, 3);
        dsp::Matrix<double> k(1, 3);

        dsp::Matrix<double> g(1, 3);

        for (int j = 0; j < 3; j++) {
            std::cout << "P Matrix:" << p(0, j) << "\n";

        }

        for (int j = 0; j < 3; j++) {
            m(0, j) = a(0, j);
            m(1, j) = b(0, j);
            m(2, j) = c(0, j);
        }
        for (int i = 0; i < 3; i++) {
            std::cout << " | ";
            for (int j = 0; j < 3; j++) {
                std::cout << "  " << m(i, j);
            }
            std::cout << " | \n";
        }
        // computes the inverse of a matrix m
        double det = m(0, 0) * (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) -
                     m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
                     m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));

        if (det == 0) {
            Mat t(1, 3);
            t(0, 0) = 0;
            t(0, 1) = 0;
            t(0, 2) = 0;
            return t;
        }
        double invdet = 1 / det;


        minv(0, 0) = (m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2)) * invdet;
        minv(0, 1) = (m(0, 2) * m(2, 1) - m(0, 1) * m(2, 2)) * invdet;
        minv(0, 2) = (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * invdet;
        minv(1, 0) = (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) * invdet;
        minv(1, 1) = (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0)) * invdet;
        minv(1, 2) = (m(1, 0) * m(0, 2) - m(0, 0) * m(1, 2)) * invdet;
        minv(2, 0) = (m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1)) * invdet;
        minv(2, 1) = (m(2, 0) * m(0, 1) - m(0, 0) * m(2, 1)) * invdet;
        minv(2, 2) = (m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1)) * invdet;

        g = p * minv;

        std::cout << "---------Interpolation Gains-------" << " | \n";
        for (int j = 0; j < 3; j++) {
            std::cout << " | " << g(0, j) << " | \n";
        }

        return g;
    }

    /*=================================================================================*/
    void loadPlayer(String filename, Player player){

        AudioPlayer temp = loadAudioFilePlayer(filename, .80f);
        player.audioPlayer.buffer = temp.buffer;
        player.audioPlayer.gain = temp.gain;


        player.currentPos.x = 0.0f;
        player.currentPos.y = 8.0f;
        Position posTemp;

        player.buildRoute(player.currentPos);
        posTemp.x += 0.0;
        posTemp.y += 8.0f;
        player.addToRoute(posTemp);

        while(posTemp.y < 10.0f) {
            posTemp.x += 0.25;
            posTemp.y += 0.5f;
            player.addToRoute(posTemp);
            //std::cout << "Azimuth: " << vectorToSphere(posTemp).azimuth << "\n";
            //std::cout << "Radius: " << vectorToSphere(posTemp).radius << "\n";
        }
        while(posTemp.y < 25.0f) {
            posTemp.x += 0.5f;
            posTemp.y += 1.0f;
            player.addToRoute(posTemp);
            //std::cout << "Azimuth: " << vectorToSphere(posTemp).azimuth << "\n";
            //std::cout << "Radius: " << vectorToSphere(posTemp).radius << "\n";
        }
        while(posTemp.y >= 8.0f) {
            posTemp.x -= 1.5f;
            posTemp.y -= 1.0f;
            player.addToRoute(posTemp);
            //std::cout << "Azimuth: " << vectorToSphere(posTemp).azimuth << "\n";
            //std::cout << "Radius: " << vectorToSphere(posTemp).radius << "\n";
        }

        while(posTemp.x >= -20.0f) {
            posTemp.x -= .5f;
            player.addToRoute(posTemp);
            //std::cout << "Azimuth: " << vectorToSphere(posTemp).azimuth << "\n";
            //std::cout << "Radius: " << vectorToSphere(posTemp).radius << "\n";
        }

        while(posTemp.x <= 0) {
            posTemp.x += .5f;
            player.addToRoute(posTemp);
            //std::cout << "Azimuth: " << vectorToSphere(posTemp).azimuth << "\n";
            //std::cout << "Radius: " << vectorToSphere(posTemp).radius << "\n";
        }
        players.push_back(player);

    }

/*=================================================================================*/
    void followRoute(Player &player){
        static float distance = 0;
        relativeTime1 += relativeTime1.milliseconds(10).inMilliseconds();
        if ( relativeTime1.inMilliseconds() > 1000000.0f/3.0f) {
            player.head = player.head->next;
            std::cout << "Path Node: x: " << player.head->current.x << "y:" << player.head->current.y << "\n";
            std::cout << "Path Node: Azimuth: " << vectorToSphere(player.head->current).azimuth << "\n";
            std::cout << "Path Node: Radius: " << vectorToSphere(player.head->current).radius << "\n";
            player.gain =  3/vectorToSphere(player.head->current).radius;
            //Process gain for distance
            std::cout << "Player Gain: " << .4 + player.gain << "\n";

            //gainProcessorPlayer.processBlock(player.audioPlayer.buffer,emptyMidi);

            auto hr = findClosestHRTF(vectorToSphere(player.head->current).azimuth);
            player.hrtfIndex =hr;
            player.bufferCurrent = zeroPlane.at(hr);
            //std::cout << "HRTF index " << player.hrtfIndex << "\n";
            relativeTime1 = relativeTime1.milliseconds(0);
            distance = vectorToSphere(player.head->current).radius;
        }

    }

    /*=================================================================================*/
    void findNearest(Player &player, int index){


        for(int i =0; i < player.steps; i++)
            player.head = player.head->next;
//                std::cout << "Path Node: x: " << player.head->current.x << "y:" << player.head->current.y << "\n";
//                std::cout << "Path Node: Azimuth: " << vectorToSphere(player.head->current).azimuth << "\n";
//                std::cout << "Path Node: Radius: " << vectorToSphere(player.head->current).radius << "\n";
        player.gain=  4/ vectorToSphere(player.head->current).radius;
        auto hr = findClosestHRTF(vectorToSphere(player.head->current).azimuth);
        player.hrtfIndex =hr;
        player.bufferCurrent = zeroPlane.at(hr);
//                std::cout << "HRTF index " << player.hrtfIndex << "\n";


    }

    //=========================================================================
    //========================Variables=========================================
    RelativeTime position;
    RelativeTime relativeTime1;
    double sampleRate = 44100.0;
    MidiBuffer emptyMidi;
    int samplesExpected;
    RelativeTime relativeTime;
    int impulseIndex = 0;
    int degrees = 0;

    std::vector<const int> elevations = {-45, -39, -34, -28, -23, -17, -11, -6, 0, 6, 11,
                                         17, 23, 28, 34, 39, 45, 51, 56, 62, 68, 73, 79,
                                         84, 90, 96, 101, 107, 113, 118, 124, 129, 135, 141,
                                         146, 152, 158, 163, 169, 174, 180, 186, 191, 197,
                                         203, 208, 214, 219, 225, 231};


//========================Gui Elements=========================================
    TextButton openButton;
    TextButton playButton;
    TextButton stopButton;
    ToggleButton loopingToggle;
    Label currentPositionLabel;
    Slider frequencySlider;
    Label frequencyLabel;
    Slider durationSlider;
    Label durationLabel;
    ToggleButton homeButton;
    Label homeLabel;
    ToggleButton awayButton;
    Label awayLabel;

    Label azimuthPosition;
    Slider azimuthSlider;

    //====================File and Resource loading=========================================
    AudioFormatManager formatManager;
    AudioFormatManager formatManager1;
    std::unique_ptr<AudioFormatReaderSource> readerSource;
    std::unique_ptr<AudioFormatReaderSource> readerSource1;

//=====================Audio Sources=====================================================
    std::vector<AudioPlayer> audioList;
    std::unique_ptr<AudioTransportSource> transportSource;
    TransportState state;

    //=====================Effects and processing=====================================================
    FilterProcessor filter;
    std::unique_ptr<ConvolutionProcessor> convolutionProcessor;
    std::unique_ptr<AudioSampleBuffer> inputL;
    std::unique_ptr<AudioSampleBuffer> inputR;

    //=====================HRTF buffers and data stuctures=====================================================
    std::vector<AudioSampleBuffer> rightVec;
    std::vector<AudioSampleBuffer> leftVec;
    std::vector<AudioSampleBuffer> rightVecNeg;
    std::vector<AudioSampleBuffer> leftVecNeg;
    std::vector<AudioSampleBuffer> leftHRIR;
    std::vector<AudioSampleBuffer> rightHRIR;
    std::vector<HRTFData> plusSix;
    std::vector<HRTFData> minusSix;
    std::vector<HRTFData> zeroPlane;
    std::map<int, std::map<int, HRTFData>> hrtfMap;

    //Gain Effects variables
    float rawVolume;

    std::vector<Player> players;
    Player one;
    int indexPast;
    int lastAzimuthPos;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};