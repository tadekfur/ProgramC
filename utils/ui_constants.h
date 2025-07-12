#pragma once

namespace UIConstants {
    // Sidebar
    constexpr int SIDEBAR_WIDTH = 260;
    constexpr int SIDEBAR_SPACING = 18;
    constexpr int SIDEBAR_MARGINS = 18;
    constexpr int SIDEBAR_TOP_MARGIN = 32;
    
    // Buttons
    constexpr int BUTTON_MIN_HEIGHT = 44;
    constexpr int BUTTON_BORDER_RADIUS = 10;
    constexpr int BUTTON_FONT_SIZE = 18;
    
    // Colors (use QString for easy CSS integration)
    inline const QString COLOR_PRIMARY = "#1e40af";
    inline const QString COLOR_PRIMARY_HOVER = "#2563eb";
    inline const QString COLOR_SECONDARY = "#e0edff";
    inline const QString COLOR_BACKGROUND = "#f6f8fa";
    inline const QString COLOR_SIDEBAR = "#e5e7eb";
    inline const QString COLOR_TEXT_PRIMARY = "#222";
    inline const QString COLOR_WHITE = "#fff";
    
    // Legend colors
    inline const QString COLOR_OVERDUE = "#ef4444";
    inline const QString COLOR_TODAY = "#fde047";
    inline const QString COLOR_ONE_DAY = "#fb923c";
    inline const QString COLOR_TWO_DAYS = "#60a5fa";
    inline const QString COLOR_THREE_DAYS = "#22c55e";
    inline const QString COLOR_FOUR_DAYS = "#9ca3af";
}
