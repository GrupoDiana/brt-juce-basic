# BRT Library Example with JUCE
Este repositorio contiene un ejemplo de uso de la [BRT Library](https://github.com/GrupoDiana/BRTLibrary) con el framework [JUCE](https://github.com/juce-framework/JUCE). Ambas BRT Library y JUCE están incluidas como submódulos git en el directorio [Libs](Libs/).
## Generación de proyectos con Projucer
Para generar los archivos de proyecto para cada plataforma, necesitarás usar Projucer, que es una herramienta de JUCE. Sigue estos pasos:
1.	Asegúrate de tener Projucer. Si no lo tienes puedes compilarlo para tu plataforma [(Libs/JUCE/extras/Projucer/Builds)](Libs/JUCE/extras/Projucer/Builds)
2.  Abre Projucer.
3.	Haz clic en "Open...".
4.	Navega hasta el directorio del repositorio y selecciona el archivo [brt-juce-basic.jucer](brt-juce-basic.jucer). A continuación tienes que editar los directorios donde te has descargado todo, que serán distintos a los que están guardadoss en el proyecto. 
5.  Edita en "File > Global Paths..." los directorios donde está JUCE y sus submódulos. 
6.  En la ventana vertical de la izquierda, entra en Exporters y edita los directorios donde están los includes y las librerías adicionales. 
7.	En la barra de menús de Projucer, selecciona "Guardar y abrir en IDE".
8.	Selecciona el IDE que prefieras. Projucer generará los archivos de proyecto en el directorio [Builds/](Builds/).
Recuerda que cada vez que hagas cambios en el archivo .jucer, deberás repetir estos pasos para actualizar los archivos de proyecto.
## Uso de la BRT Library y JUCE
El archivo brt-juce-basic.h es el punto de entrada principal para este ejemplo, y en el se realizan todas las tareas de configuración y uso de la BRT Library. 

### Configuración de la BRT Library
La BRT Library se configura en el método `setupBRT(int sampleRate, int bufferSize)`. Aquí, se establecen los parámetros globales de la biblioteca, como la frecuencia de muestreo y el tamaño del buffer. También se crea un objeto listener, que representa al oyente en el espacio 3D.
### Carga de archivos SOFA
Los archivos SOFA, que contienen las respuestas al impulso de la cabeza relacionadas con la transferencia (HRTF), se cargan en el método  `LoadSOFAFile(const juce::File& file)`. Estos archivos se utilizan para proporcionar las HRTF que se utilizarán para el procesamiento binaural.
### Creación y posicionamiento de fuentes de sonido
Las fuentes de sonido se crean y posicionan en el método  `c++ LoadSource(const String& name, float azimuth, float elevation, float distance)`. Aquí, se crea una fuente de sonido y se conecta al oyente. Luego, se establece la posición de la fuente en el espacio 3D.
### Procesamiento de audio
El procesamiento de audio se realiza en el método  `c++ getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)`. Aquí, se obtienen las muestras de audio de la fuente, se pasan a la BRT Library y se procesan todas las fuentes. Luego, se obtiene el buffer de salida estéreo y se envía al dispositivo de salida de audio.
### Control de la reproducción
La reproducción del audio se controla mediante los métodos  `c++ playButtonClicked()` y  `c++ stopButtonClicked()`, que inician y detienen la reproducción, respectivamente.