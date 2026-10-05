import os
from playwright.sync_api import sync_playwright

def render():
    html_path = os.path.abspath(r"c:\APPS-DEV\zzz\KTM-Bus\hardware\wiring_diagram_renderer.html")
    output_jpg = os.path.abspath(r"c:\APPS-DEV\zzz\KTM-Bus\hardware\ktm_wiring_diagram_v2.jpg")
    output_png = os.path.abspath(r"c:\APPS-DEV\zzz\KTM-Bus\hardware\ktm_wiring_diagram_v2.png")
    
    with sync_playwright() as p:
        browser = p.chromium.launch()
        page = browser.new_page(viewport={"width": 1920, "height": 1080}, device_scale_factor=2)
        page.goto(f"file:///{html_path.replace(os.sep, '/')}")
        page.wait_for_timeout(1000)
        page.screenshot(path=output_png, full_page=True)
        page.screenshot(path=output_jpg, type="jpeg", quality=95, full_page=True)
        browser.close()
    print("Render complete! Images saved to:")
    print(" -", output_jpg)
    print(" -", output_png)

if __name__ == "__main__":
    render()
