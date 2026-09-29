# 截图编辑器的滤镜、聚光灯、选区比例与 OCR 表格模块
set(MARK_SHOT_EDITOR_FEATURE_SOURCES
    src/annotation_filters/image_filter_effects.cpp
    src/annotation_filters/image_filter_effects.h
    src/annotation_filters/spotlight_overlay.cpp
    src/annotation_filters/spotlight_overlay.h
    src/ocr_table/ocr_table_layout.cpp
    src/ocr_table/ocr_table_layout.h
    src/ocr_table/ocr_table_pane.cpp
    src/ocr_table/ocr_table_pane.h
    src/selection_aspect/selection_aspect_ratio.cpp
    src/selection_aspect/selection_aspect_ratio.h
    src/selection_aspect/selection_size_panel.cpp
    src/selection_aspect/selection_size_panel.h
    src/shot_window_filter_rendering.cpp
    src/shot_window_selection_size.cpp
    src/shot_window_style_combos.cpp
)
