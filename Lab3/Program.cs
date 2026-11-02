using System;
using System.Linq;
using System.Collections.Generic;
using System.Threading;

namespace EmpiricalLab3
{
    class Program
    {
        static void Main()
        {
            Thread.CurrentThread.CurrentCulture = new System.Globalization.CultureInfo("en-US");
            Console.OutputEncoding = System.Text.Encoding.UTF8;
            Console.ForegroundColor = ConsoleColor.Black;
            Console.BackgroundColor = ConsoleColor.White;
            Console.Clear();

            Console.WriteLine("================ Перевірка алгоритму моди ================");
            TestModeAlgorithm();
            Console.WriteLine("==========================================================\n");

            while (true)
            {
                Console.WriteLine("Введіть ваш номер варіанта N (або 0 для виходу)");
                string inputN = Console.ReadLine();

                if (!int.TryParse(inputN, out int n) || n < 0)
                {
                    Console.WriteLine("Некоректний ввід числа.");
                    continue;
                }

                if (n == 0)
                {
                    Console.WriteLine("Роботу завершено.");
                    break;
                }

                int size = n + 10;
                int[] data = new int[size];

                Console.WriteLine("Оберіть спосіб формування вибірки");
                Console.WriteLine("1 - Згенерувати випадково за варіантом");
                Console.WriteLine("2 - Ввести власну вибірку вручну через пробіл");

                string choice = Console.ReadLine();

                if (choice == "1")
                {
                    Random rnd = new Random();
                    int maxVal = n + 1;

                    for (int i = 0; i < size; i++)
                    {
                        data[i] = rnd.Next(1, maxVal + 1);
                    }
                }
                else if (choice == "2")
                {
                    Console.WriteLine($"Введіть {size} чисел через пробіл в одному рядку");
                    string[] parts = Console.ReadLine().Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);

                    if (parts.Length != size)
                    {
                        Console.WriteLine($"Помилка. Очікувалося {size} чисел, отримано {parts.Length}.");
                        continue;
                    }

                    bool error = false;
                    for (int i = 0; i < size; i++)
                    {
                        if (!int.TryParse(parts[i], out data[i]))
                        {
                            error = true;
                            break;
                        }
                    }

                    if (error)
                    {
                        Console.WriteLine("Помилка при читанні чисел.");
                        continue;
                    }
                }
                else
                {
                    Console.WriteLine("Некоректний вибір.");
                    continue;
                }

                Console.WriteLine("\n--- Вихідна послідовність ---");
                Console.WriteLine(string.Join(" ", data));

                Array.Sort(data);

                Console.WriteLine("\n--- Упорядкована послідовність (варіаційний ряд) ---");
                Console.WriteLine(string.Join(" ", data));

                PrintFrequencyTable(data);

                double mean = CalculateMean(data);
                double median = CalculateMedian(data);

                int modeFreq;
                string mode = CalculateMode(data, out modeFreq);

                Console.WriteLine("\n--- Основні показники вибірки (МЦТ) ---");
                Console.WriteLine($"Середнє арифметичне дорівнює {mean:F4}");
                Console.WriteLine($"Медіана дорівнює {median:F2}");

                if (mode == "Мода відсутня" || mode == "Немає даних")
                {
                    Console.WriteLine(mode);
                }
                else
                {
                    Console.WriteLine($"Мода дорівнює {mode} (частота: {modeFreq})");
                }
                Console.WriteLine("---------------------------------------\n");
            }
        }

        public static void PrintFrequencyTable(int[] sortedData)
        {
            Console.WriteLine("\n--- Таблиця частот ---");
            Console.WriteLine(string.Format("{0,10} | {1,10}", "Варіанта", "Частота"));
            Console.WriteLine(new string('-', 25));

            var groups = sortedData.GroupBy(x => x);
            foreach (var group in groups)
            {
                Console.WriteLine(string.Format("{0,10} | {1,10}", group.Key, group.Count()));
            }
        }

        public static double CalculateMean(int[] data)
        {
            double sum = 0;
            foreach (int val in data)
            {
                sum += val;
            }
            return sum / data.Length;
        }

        public static double CalculateMedian(int[] data)
        {
            int n = data.Length;
            if (n % 2 != 0)
            {
                return data[n / 2];
            }
            else
            {
                return (data[(n / 2) - 1] + data[n / 2]) / 2.0;
            }
        }

        public static string CalculateMode(int[] data, out int maxFrequency)
        {
            maxFrequency = 0;
            if (data.Length == 0) return "Немає даних";

            var groups = data.GroupBy(x => x)
                             .Select(g => new { Value = g.Key, Count = g.Count() })
                             .OrderBy(g => g.Value)
                             .ToList();

            // Створюємо локальну змінну для безпечного використання у лямбда-виразах
            int localMaxFreq = groups.Max(g => g.Count);

            // Одразу передаємо знайдене значення у вихідний параметр
            maxFrequency = localMaxFreq;

            // Використовуємо локальну змінну замість out-параметра
            if (groups.All(g => g.Count == localMaxFreq))
            {
                return "Мода відсутня";
            }

            var modeGroups = new List<List<int>>();
            var currentGroup = new List<int>();

            for (int i = 0; i < groups.Count; i++)
            {
                // Тут також використовуємо локальну змінну
                if (groups[i].Count == localMaxFreq)
                {
                    currentGroup.Add(groups[i].Value);
                }
                else
                {
                    if (currentGroup.Count > 0)
                    {
                        modeGroups.Add(new List<int>(currentGroup));
                        currentGroup.Clear();
                    }
                }
            }
            if (currentGroup.Count > 0)
            {
                modeGroups.Add(currentGroup);
            }

            var resultModes = new List<string>();
            foreach (var grp in modeGroups)
            {
                if (grp.Count == 1)
                {
                    resultModes.Add(grp[0].ToString());
                }
                else
                {
                    double avg = grp.Average();
                    resultModes.Add(avg.ToString("0.##", System.Globalization.CultureInfo.InvariantCulture));
                }
            }

            return string.Join(" та ", resultModes);
        }

        public static void TestModeAlgorithm()
        {
            // Масив анонімних об'єктів для зручного налаштування тестових випадків
            var testCases = new[]
            {
                new { Desc = "1 мода", Data = new[] { 1, 2, 2, 3, 4 }, ExpMode = "2", ExpFreq = 2 },
                new { Desc = "2 суміжні моди", Data = new[] { 2, 2, 3, 3, 4, 5 }, ExpMode = "2.5", ExpFreq = 2 },
                new { Desc = "2 несуміжні моди", Data = new[] { 2, 2, 3, 4, 4, 5 }, ExpMode = "2 та 4", ExpFreq = 2 },
                new { Desc = "Мода відсутня (усі по 2)", Data = new[] { 2, 2, 3, 3, 4, 4, 5, 5 }, ExpMode = "Мода відсутня", ExpFreq = 2 },
                new { Desc = "Мода відсутня (усі різні)", Data = new[] { 1, 2, 3, 4, 5 }, ExpMode = "Мода відсутня", ExpFreq = 1 },
                new { Desc = "3 несуміжні моди", Data = new[] { 1, 1, 2, 4, 4, 5, 7, 7 }, ExpMode = "1 та 4 та 7", ExpFreq = 2 },
                new { Desc = "Змішані (суміжні+несум.)", Data = new[] { 1, 1, 2, 2, 3, 4, 4 }, ExpMode = "1.5 та 4", ExpFreq = 2 }
            };

            Console.WriteLine(new string('-', 116));
            Console.WriteLine(string.Format("{0,-25} | {1,-20} | {2,-23} | {3,-18} | {4,-15}",
                "Тип вибірки", "Масив", "Очікувана мода (Част)", "Фактична мода", "Факт. частота"));
            Console.WriteLine(new string('-', 116));

            foreach (var tc in testCases)
            {
                int[] arr = tc.Data;
                Array.Sort(arr);

                int actualFreq;
                string actualMode = CalculateMode(arr, out actualFreq);

                string arrStr = string.Join(",", arr);
                string expStr = $"{tc.ExpMode} ({tc.ExpFreq})";

                Console.WriteLine(string.Format("{0,-25} | {1,-20} | {2,-23} | {3,-18} | {4,-15}",
                    tc.Desc, arrStr, expStr, actualMode, actualFreq));
            }
            Console.WriteLine(new string('-', 116));
        }
    }
}