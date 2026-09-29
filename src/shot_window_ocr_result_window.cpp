#include "shot_window_module.h"

#include "ocr_result_window/ocr_result_window.h"

#include <utility>

namespace markshot::shot {

QWidget *createOcrResultWindow(QString text,
                               QScreen *targetScreen,
                               QImage sourceImage,
                               QVector<markshot::ocr::Token> tokens)
{
    return new OcrResultWindow(std::move(text), targetScreen, std::move(sourceImage), std::move(tokens));
}

}  // namespace markshot::shot
