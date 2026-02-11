using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;

namespace OakPanel.Controls
{
    public partial class ColorPickerDialog : Window
    {
        public Color SelectedColor { get; private set; }
        private double _hue = 0;
        private double _saturation = 1;
        private double _value = 1;

        public ColorPickerDialog(Color initialColor)
        {
            InitializeComponent();
            SelectedColor = initialColor;
            RgbToHsv(initialColor, out _hue, out _saturation, out _value);
            HueSlider.Value = _hue;
            ValueSlider.Value = _value;
            UpdateColor();
        }

        private void UpdateColor()
        {
            SelectedColor = HsvToRgb(_hue, _saturation, _value);
            ColorPreview.Background = new SolidColorBrush(SelectedColor);
            UpdateRgbText();
            DrawColorCanvas();
        }

        private void DrawColorCanvas()
        {
            ColorCanvas.Children.Clear();
            var width = ColorCanvas.ActualWidth;
            var height = ColorCanvas.ActualHeight;
            
            if (width == 0 || height == 0) return;

            for (int x = 0; x < width; x++)
            {
                for (int y = 0; y < height; y++)
                {
                    double s = x / width;
                    double v = 1.0 - (y / height);
                    var color = HsvToRgb(_hue, s, v);
                    var rect = new Rectangle
                    {
                        Width = 1,
                        Height = 1,
                        Fill = new SolidColorBrush(color)
                    };
                    Canvas.SetLeft(rect, x);
                    Canvas.SetTop(rect, y);
                    ColorCanvas.Children.Add(rect);
                }
            }
        }

        private void ColorCanvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            UpdateColorFromCanvas(e.GetPosition(ColorCanvas));
        }

        private void ColorCanvas_MouseMove(object sender, MouseEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed)
            {
                UpdateColorFromCanvas(e.GetPosition(ColorCanvas));
            }
        }

        private void UpdateColorFromCanvas(Point pos)
        {
            var width = ColorCanvas.ActualWidth;
            var height = ColorCanvas.ActualHeight;
            if (width == 0 || height == 0) return;

            _saturation = Math.Max(0, Math.Min(1, pos.X / width));
            _value = Math.Max(0, Math.Min(1, 1.0 - (pos.Y / height)));
            UpdateColor();
        }

        private void HueSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            _hue = e.NewValue;
            UpdateColor();
        }

        private void ValueSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            _value = e.NewValue;
            UpdateColor();
        }

        private void ColorCanvas_Loaded(object sender, RoutedEventArgs e)
        {
            DrawColorCanvas();
        }

        private void ColorCanvas_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            DrawColorCanvas();
        }

        private void UpdateRgbText()
        {
            RgbText.Text = $"{SelectedColor.R}, {SelectedColor.G}, {SelectedColor.B}";
        }

        private Color HsvToRgb(double h, double s, double v)
        {
            h = h % 360;
            double c = v * s;
            double x = c * (1 - Math.Abs((h / 60) % 2 - 1));
            double m = v - c;

            double r = 0, g = 0, b = 0;

            if (h < 60) { r = c; g = x; b = 0; }
            else if (h < 120) { r = x; g = c; b = 0; }
            else if (h < 180) { r = 0; g = c; b = x; }
            else if (h < 240) { r = 0; g = x; b = c; }
            else if (h < 300) { r = x; g = 0; b = c; }
            else { r = c; g = 0; b = x; }

            return Color.FromRgb(
                (byte)((r + m) * 255),
                (byte)((g + m) * 255),
                (byte)((b + m) * 255)
            );
        }

        private void RgbToHsv(Color color, out double h, out double s, out double v)
        {
            double r = color.R / 255.0;
            double g = color.G / 255.0;
            double b = color.B / 255.0;

            double max = Math.Max(r, Math.Max(g, b));
            double min = Math.Min(r, Math.Min(g, b));
            double delta = max - min;

            v = max;
            s = max > 0.001 ? delta / max : 0;

            if (delta < 0.001)
            {
                h = 0;
            }
            else if (max == r)
            {
                h = 60 * (((g - b) / delta) % 6);
            }
            else if (max == g)
            {
                h = 60 * (((b - r) / delta) + 2);
            }
            else
            {
                h = 60 * (((r - g) / delta) + 4);
            }

            if (h < 0) h += 360;
        }

        private void OkButton_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
            Close();
        }

        private void CancelButton_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
            Close();
        }

        private void CloseButton_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
            Close();
        }
    }
}
