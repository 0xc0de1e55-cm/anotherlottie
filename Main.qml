import QtQuick
import QtQuick.Controls

import anotherlottie

Window {
    width: 200
    height: 200
    visible: true
    title: qsTr("Another lottie example")
    color: "black"

    component Lottie : AnotherLottie {
        width: 100
        height: 100
        autoPlay: true
        loops: AnotherLottie.Infinite
    }

    function sticker(name) {
        return "file:../../stickers/%1.json".arg(name);
    }

    Flow {
        anchors.fill: parent

        Lottie { source: sticker("sticker1") }
        Lottie { source: sticker("sticker2") }
        Lottie { source: sticker("sticker3") }
        Lottie { source: sticker("sticker4") }
    }
}
