add_rules("mode.debug", "mode.release")
add_requires("gtest")
set_targetdir("build")
target("GoatScreenShot")
    add_rules("qt.shared")
    add_frameworks("QtCore", "QtGui", "QtWidgets")
    if is_plat("mingw") then 
        add_links("kernel32", "user32", "gdi32")
    end
    if is_os("windows") then 
        add_frameworks("QtWinExtras")
    end
    add_defines("GoatScreenShot_LIB")
    add_headerfiles("include/GoatScreenshot.h")
    add_headerfiles("include/GoatScreenshotGDI.h")
    add_includedirs("include")
    add_files("src/GoatScreenshot.cpp") 
    if is_os("windows") then 
        add_files("src/GoatScreenshotGDI.cpp") 
        add_files("src/GoatScreenshotDXGI.cpp")
    elseif is_os("macosx") then
        add_files("src/GoatScreenshotMac.cpp")
    end
    add_files("src/ImageHandle.cpp") 
    
 
target("test") 
    add_rules("qt.console")
    add_packages("gtest")
    add_frameworks("QtCore", "QtGui", "QtWidgets")
    add_deps("GoatScreenShot")
    add_includedirs("include")
    add_files("test/*.cpp") 