# Lotus Notebook Python Runtime Setup
# This script configures Python environment for Lotus Notebook

# Configure matplotlib for inline display
import matplotlib
matplotlib.use('Agg')

# Import required libraries
import sys
import io
import base64
import json
import traceback as traceback_module

# Try to import markdown library for markdown rendering support
try:
    import markdown
    MARKDOWN_AVAILABLE = True
except ImportError:
    MARKDOWN_AVAILABLE = False
    print("Note: markdown library not available. Install with: pip install markdown")

# Import re for simple markdown rendering
import re

# Output types for rich display
class LotusOutput:
    """Class to represent different output types"""
    TEXT = "text"
    HTML = "html"
    MARKDOWN = "markdown"
    SVG = "svg"
    PNG = "png"
    JPEG = "jpeg"
    TABLE = "table"
    JSON = "json"
    ERROR = "error"
    RICH = "rich"

class LotusDisplay:
    """
    Class for rich display in Lotus Notebook.
    Mimics IPython's display system.
    """
    def __init__(self):
        self._last_html = None
        self._last_svg = None

    def display(self, obj, **kwargs):
        """Display an object"""
        if isinstance(obj, str):
            if obj.startswith('<') and ('<table' in obj or '<div' in obj or obj.startswith('<')):
                # HTML content
                return self._create_output(LotusOutput.HTML, obj)
            else:
                return self._create_output(LotusOutput.TEXT, obj)
        elif hasattr(obj, '_repr_html_'):
            return self._create_output(LotusOutput.HTML, obj._repr_html_())
        elif hasattr(obj, '_repr_svg_'):
            return self._create_output(LotusOutput.SVG, obj._repr_svg_())
        elif hasattr(obj, '_repr_markdown_'):
            return self._create_output(LotusOutput.MARKDOWN, obj._repr_markdown_())
        elif hasattr(obj, '_repr_json_'):
            return self._create_output(LotusOutput.JSON, json.dumps(obj._repr_json_()))
        elif hasattr(obj, '_repr_png_'):
            return self._create_output(LotusOutput.PNG, obj._repr_png_())
        elif hasattr(obj, '_repr_jpeg_'):
            return self._create_output(LotusOutput.JPEG, obj._repr_jpeg_())
        else:
            return self._create_output(LotusOutput.TEXT, repr(obj))

    def _create_output(self, output_type, content):
        """Create output dictionary"""
        output = {
            'type': output_type,
            'content': content
        }
        print(f"LOTUS_OUTPUT:{json.dumps(output)}")
        return output

    def clear(self):
        """Clear the display"""
        print("LOTUS_CLEAR")

# Global display instance
display = LotusDisplay()

def setup_lotus_environment():
    """
    Setup the Python environment for Lotus Notebook.
    This is automatically called when the executor initializes.
    """
    # Print welcome message
    print("Lotus Notebook Python Environment Ready")
    print(f"Python Version: {sys.version}")
    print()

    # Check available libraries
    try:
        import numpy
        print(f"NumPy Version: {numpy.__version__}")
    except ImportError:
        print("NumPy: Not available")

    try:
        import pandas
        print(f"Pandas Version: {pandas.__version__}")
    except ImportError:
        print("Pandas: Not available")

    try:
        import matplotlib
        print(f"Matplotlib Version: {matplotlib.__version__}")
    except ImportError:
        print("Matplotlib: Not available")

    try:
        import sklearn
        print(f"Scikit-learn Version: {sklearn.__version__}")
    except ImportError:
        print("Scikit-learn: Not available")

    print()
    print("Type your Python code below and press Ctrl+Enter to execute.")
    print("Use %timeit for timing, %run for running scripts, etc.")

def format_dataframe(df):
    """
    Format a pandas DataFrame as an HTML table.
    """
    try:
        import pandas as pd
        if isinstance(df, pd.DataFrame):
            # Get basic info
            info = {
                'shape': df.shape,
                'columns': list(df.columns),
                'dtypes': {col: str(dtype) for col, dtype in df.dtypes.items()}
            }

            # Format as HTML with styling
            html = df.to_html(classes='lotus-dataframe', index=False, border=0)
            return html
    except:
        pass
    return None

def format_series(series):
    """
    Format a pandas Series as an HTML table.
    """
    try:
        import pandas as pd
        if isinstance(series, pd.Series):
            html = f"""
            <div class="lotus-series">
                <div class="lotus-series-name">{series.name}</div>
                {series.to_frame().to_html(classes='lotus-dataframe', border=0)}
            </div>
            """
            return html
    except:
        pass
    return None

def capture_plot():
    """
    Capture the current matplotlib figure and return as base64-encoded PNG.
    This function is called by the C++ executor to get plot images.
    """
    try:
        import matplotlib.pyplot as plt
        from io import BytesIO

        # Save figure to buffer
        buf = BytesIO()
        plt.savefig(buf, format='png', bbox_inches='tight', dpi=100)
        buf.seek(0)

        # Encode to base64
        img_base64 = base64.b64encode(buf.read()).decode('utf-8')

        # Clear the figure
        plt.clf()
        plt.cla()
        plt.close('all')

        return img_base64
    except Exception as e:
        return None

def capture_svg():
    """
    Capture the current matplotlib figure as SVG.
    """
    try:
        import matplotlib.pyplot as plt
        from io import StringIO

        # Save figure to buffer
        buf = StringIO()
        plt.savefig(buf, format='svg', bbox_inches='tight')
        buf.seek(0)

        svg_content = buf.read()

        # Clear the figure
        plt.clf()
        plt.cla()
        plt.close('all')

        return svg_content
    except Exception as e:
        return None

def clear_plots():
    """
    Clear all current matplotlib figures.
    """
    try:
        import matplotlib.pyplot as plt
        plt.clf()
        plt.cla()
        plt.close('all')
    except:
        pass

def format_exception(error):
    """
    Format an exception with syntax highlighting for traceback.
    Returns HTML-formatted error message.
    """
    html = f"""
    <div class="lotus-error">
        <div class="lotus-error-type">{type(error).__name__}: {str(error)}</div>
        <pre class="lotus-traceback">
{traceback.format_exc()}
        </pre>
    </div>
    """
    return html

def render_markdown(text):
    """
    Render markdown text to HTML.
    Uses Python's markdown library if available, otherwise falls back to basic formatting.
    Returns LOTUS_OUTPUT with markdown type.
    """
    if not MARKDOWN_AVAILABLE:
        # Fallback: simple text with basic formatting
        html = _simple_markdown_render(text)
    else:
        # Use full markdown library with extensions
        md = markdown.Markdown(
            extensions=['fenced_code', 'tables', 'codehilite', 'nl2br'],
            extension_configs={
                'codehilite': {
                    'css_class': 'highlight'
                }
            }
        )
        html = md.convert(text)

    # Wrap in container with styling
    styled_html = f'''
    <style>
        .lotus-markdown-content {{
            font-family: Arial, sans-serif;
            font-size: 12px;
            line-height: 1.6;
            color: #333;
        }}
        .lotus-markdown-content h1 {{
            font-size: 24px;
            border-bottom: 1px solid #e0e0e0;
            padding-bottom: 8px;
            margin-bottom: 16px;
        }}
        .lotus-markdown-content h2 {{
            font-size: 20px;
            border-bottom: 1px solid #e0e0e0;
            padding-bottom: 6px;
            margin-bottom: 14px;
        }}
        .lotus-markdown-content h3 {{
            font-size: 16px;
            margin-bottom: 12px;
        }}
        .lotus-markdown-content pre {{
            background-color: #f5f5f5;
            padding: 12px;
            border-radius: 4px;
            overflow-x: auto;
        }}
        .lotus-markdown-content code {{
            background-color: #f5f5f5;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: 'Fira Code', monospace;
            font-size: 11px;
        }}
        .lotus-markdown-content pre code {{
            background-color: transparent;
            padding: 0;
        }}
        .lotus-markdown-content blockquote {{
            border-left: 4px solid #2E7D32;
            margin: 0;
            padding-left: 16px;
            color: #666;
        }}
        .lotus-markdown-content a {{
            color: #1976d2;
            text-decoration: none;
        }}
        .lotus-markdown-content hr {{
            border: none;
            border-top: 1px solid #e0e0e0;
            margin: 16px 0;
        }}
        .lotus-markdown-content table {{
            border-collapse: collapse;
            width: 100%;
        }}
        .lotus-markdown-content th, .lotus-markdown-content td {{
            border: 1px solid #e0e0e0;
            padding: 8px 12px;
            text-align: left;
        }}
        .lotus-markdown-content th {{
            background-color: #f5f5f5;
        }}
        .lotus-markdown-content img {{
            max-width: 100%;
        }}
    </style>
    <div class="lotus-markdown-content">{html}</div>
    '''

    output = {{
        'type': 'markdown',
        'content': styled_html
    }}
    print(f"LOTUS_OUTPUT:{{json.dumps(output)}}")
    return output

def _simple_markdown_render(text):
    """
    Simple markdown-like rendering for when markdown library is not available.
    """
    html = text

    # Escape HTML
    html = html.replace("&", "&amp;")
    html = html.replace("<", "&lt;")
    html = html.replace(">", "&gt;")

    # Headers
    html = re.sub(r'^###### (.+)$', r'<h6>\1</h6>', html, flags=re.MULTILINE)
    html = re.sub(r'^##### (.+)$', r'<h5>\1</h5>', html, flags=re.MULTILINE)
    html = re.sub(r'^#### (.+)$', r'<h4>\1</h4>', html, flags=re.MULTILINE)
    html = re.sub(r'^### (.+)$', r'<h3>\1</h3>', html, flags=re.MULTILINE)
    html = re.sub(r'^## (.+)$', r'<h2>\1</h2>', html, flags=re.MULTILINE)
    html = re.sub(r'^# (.+)$', r'<h1>\1</h1>', html, flags=re.MULTILINE)

    # Bold
    html = re.sub(r'\*\*(.+?)\*\*', r'<strong>\1</strong>', html)
    html = re.sub(r'__(.+?)__', r'<strong>\1</strong>', html)

    # Italic
    html = re.sub(r'\*(.+?)\*', r'<em>\1</em>', html)
    html = re.sub(r'_(.+?)_', r'<em>\1</em>', html)

    # Code blocks
    html = re.sub(r'```(\w*)\n([\s\S]+?)```', r'<pre><code>\2</code></pre>', html)

    # Inline code
    html = re.sub(r'`(.+?)`', r'<code>\1</code>', html)

    # Lists
    html = re.sub(r'^\* (.+)$', r'<li>\1</li>', html, flags=re.MULTILINE)
    html = re.sub(r'^- (.+)$', r'<li>\1</li>', html, flags=re.MULTILINE)
    html = re.sub(r'^\d+\. (.+)$', r'<li>\1</li>', html, flags=re.MULTILINE)

    # Links
    html = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', r'<a href="\2">\1</a>', html)

    # Horizontal rules
    html = re.sub(r'^---+$', r'<hr>', html, flags=re.MULTILINE)

    # Blockquotes
    html = re.sub(r'^> (.+)$', r'<blockquote>\1</blockquote>', html, flags=re.MULTILINE)

    # Paragraphs
    paragraphs = html.split('\n\n')
    html = '</p><p>'.join(paragraphs)
    html = '<p>' + html + '</p>'

    # Line breaks within paragraphs
    html = html.replace('\n', '<br>')

    return html

def pretty_format(obj, max_depth=3, indent=2):
    """
    Pretty print a Python object with syntax highlighting.
    """
    def _format(obj, depth=0):
        if depth > max_depth:
            return "..."

        if isinstance(obj, dict):
            items = []
            for k, v in obj.items():
                items.append(f'"{k}": {_format(v, depth+1)}')
            return "{" + ", ".join(items) + "}"
        elif isinstance(obj, (list, tuple)):
            items = [_format(v, depth+1) for v in obj]
            if isinstance(obj, tuple):
                return "(" + ", ".join(items) + ")"
            else:
                return "[" + ", ".join(items) + "]"
        elif isinstance(obj, str):
            return f'"{obj}"'
        elif isinstance(obj, bool):
            return "True" if obj else "False"
        elif obj is None:
            return "None"
        elif hasattr(obj, '__repr__'):
            result = repr(obj)
            if len(result) > 100:
                result = result[:97] + "..."
            return result
        else:
            return str(obj)

    return _format(obj)

# IPython-style magic commands support
class IPythonMagic:
    """Support for common IPython magic commands"""
    def __init__(self):
        self._timing_results = {}

    def timeit(self, code, number=1000000):
        """Time code execution"""
        import time
        start = time.perf_counter()
        for _ in range(min(number, 10000)):
            eval(code)
        elapsed = time.perf_counter() - start
        return f"{elapsed:.6f} seconds"

    def run(self, filename):
        """Run a Python file"""
        with open(filename) as f:
            exec(f.read(), globals())

# Global magic instance
magic = IPythonMagic()

# Auto-setup when imported
setup_lotus_environment()
