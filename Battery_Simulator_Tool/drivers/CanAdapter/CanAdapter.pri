HEADERS  += $$PWD/CanAdapter.h \
            $$PWD/CanHub.h \
            $$PWD/CanAdapterFactory.h \
            $$PWD/can_message.h \
            $$PWD/CanAdapterChina.h \
            $$PWD/CanAdapterLoopback.h \
            $$PWD/CanAdapterTesting.h \
            $$PWD/CanAdapterPCAN.h
SOURCES  += $$PWD/CanAdapter.cpp \
$$PWD/CanAdapterFactory.cpp \
$$PWD/CanHub.cpp \
$$PWD/CanAdapterChina.cpp \
$$PWD/CanAdapterLoopback.cpp \
$$PWD/CanAdapterTesting.cpp \
$$PWD/ChinaControlWidget.cpp \
$$PWD/TestingControlWidget.cpp \
$$PWD/SerialPortComboBox.cpp \
$$PWD/PollingCanAdapter.cpp

FORMS       += \
    $$PWD/ChinaControlWidget.ui    $$PWD/TestingControlWidget.ui
