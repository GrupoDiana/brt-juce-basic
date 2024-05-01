# BRT Library Example with JUCE
This is an example of use of the [BRT Library](https://github.com/GrupoDiana/BRTLibrary) with the [JUCE](https://github.com/juce-framework/JUCE) framework. Both are included as git submodules in the [Libs](Libs/) directory, along with their dependencies [LibMySofa](https://github.com/GrupoDiana/libmysofa) and [ZLib](https://github.com/GrupoDiana/zlib), these are already compiled for various platforms.

## Project generation with Projucer
To generate the project files for each platform, you will need to use Projucer, which is a JUCE tool. Follow these steps:
1. Make sure you have Projucer. If you don't have it, you can compile it for your platform [(Libs/JUCE/extras/Projucer/Builds)](Libs/JUCE/extras/Projucer/Builds)
2. Open Projucer.
3. Click on "Open...".
4. Navigate to the repository directory and select the file [brt-juce-basic.jucer](brt-juce-basic.jucer). Then you have to edit the directories where you have downloaded everything, which will be different from the ones saved in the project: 
    - Edit in "File > Global Paths..." the directories where JUCE and its submodules are. 
    - In the vertical window on the left, enter "Exporters" and edit the directories where the includes and additional libraries are. 
5. In the Projucer menu bar, select "Save Project and Open in IDE...".
6. Select the IDE you prefer. Projucer will generate the project files in the [Builds/](Builds/) directory.
Remember that every time you make changes to the .jucer file, you will have to repeat these steps to update the project files.
## Use of the BRT Library and JUCE
The file [brt-juce-basic.h](Source/brt-juce-basic.h) is the main entry point for this example, and it performs all the configuration and use tasks of the BRT Library. 

### BRT Library Configuration
The BRT Library is configured in the `setupBRT(int sampleRate, int bufferSize)` method. Here, the global parameters of the library, such as the sampling frequency and the buffer size, are set. Also, a listener object is created, which represents the listener in the 3D space.

### Loading of SOFA files
SOFA files, which contain the head-related transfer responses (HRTF), are loaded in the `LoadSOFAFile(const juce::File& file)` method. These files are used to provide the HRTFs that will be used for binaural processing.

### Creation and positioning of sound sources
Sound sources are created and positioned in the `LoadSource(const String& name, float azimuth, float elevation, float distance)` method. Here, a sound source is created and connected to the listener. Then, the source's position in the 3D space is set.

### Audio processing
Audio processing is done in the `getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)` method. Here, the audio samples from the source are obtained, passed to the BRT Library, and all sources are processed. Then, the stereo output buffer is obtained and sent to the audio output device.

### Playback control
The audio playback is controlled by the `playButtonClicked()` and `stopButtonClicked()` methods, which start and stop the playback, respectively.
