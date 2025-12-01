#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QColor>
#include <QString>

namespace Style {
    // M3 Color System (Dark Theme) - from style.css
    namespace Colors {
        const QColor SURFACE = QColor("#141218");
        const QColor SURFACE_CONTAINER = QColor("#1D1B20");
        const QColor SURFACE_HIGH = QColor("#2B2930");
        const QColor ON_SURFACE = QColor("#E6E1E5");
        const QColor ON_SURFACE_VARIANT = QColor("#CAC4D0");
        
        const QColor PRIMARY = QColor("#D0BCFF");
        const QColor ON_PRIMARY = QColor("#381E72");
        const QColor PRIMARY_CONTAINER = QColor("#4F378B");
        const QColor ON_PRIMARY_CONTAINER = QColor("#EADDFF");
        
        const QColor SECONDARY = QColor("#CCC2DC");
        const QColor TERTIARY = QColor("#EFB8C8");
        const QColor ON_TERTIARY = QColor("#492532");
        
        const QColor BORDER = QColor("#2B2930");
    }
    
    // Spacing (pixels)
    namespace Spacing {
        const int XS = 4;
        const int SM = 8;
        const int MD = 16;
        const int LG = 24;
        const int XL = 32;
        const int XXL = 48;
    }
    
    // Typography
    namespace Typography {
        const QString FONT_FAMILY = "Outfit, SF Pro Display, Roboto, sans-serif";
        const int FONT_SIZE_SM = 12;
        const int FONT_SIZE_MD = 14;
        const int FONT_SIZE_LG = 18;
        const int FONT_SIZE_XL = 24;
        const int FONT_SIZE_XXL = 32;
    }
    
    // Border Radius
    namespace BorderRadius {
        const int XS = 4;
        const int SM = 8;
        const int MD = 12;
        const int LG = 16;
        const int XL = 28;
        const int FULL = 9999;
    }
    
    // Animation Durations (milliseconds)
    namespace Animation {
        const int FAST = 150;
        const int NORMAL = 300;
        const int SLOW = 500;
    }
}

#endif // CONSTANTS_H
