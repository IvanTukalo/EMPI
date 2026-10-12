using System;
using System.Linq;
using System.Collections.Generic;

namespace EmpiricalLab
{
    class Program
    {
        static void Main(string[] args)
        {
            Console.OutputEncoding = System.Text.Encoding.UTF8;
            Random rnd = new Random();

            while (true)
            {
                Console.WriteLine("\nВведіть номер студента N (або 0 для виходу)");
                string inputN = Console.ReadLine();
                if (!int.TryParse(inputN, out int n))
                {
                    Console.WriteLine("Помилка вводу. Спробуйте ще раз.");
                    continue;
                }

                if (n == 0)
                {
                    Console.WriteLine("Програму завершено.");
                    break;
                }

                int size = n + 10;
                int[] data = new int[size];

                Console.WriteLine($"Кількість елементів для обробки становить {size}");
                Console.WriteLine("Оберіть спосіб заповнення (1 - випадково, 2 - вручну через пробіл)");
                string choice = Console.ReadLine();

                if (choice == "1")
                {
                    for (int i = 0; i < size; i++)
                    {
                        data[i] = rnd.Next(1, 6);
                    }
                }
                else if (choice == "2")
                {
                    Console.WriteLine($"Введіть {size} елементів (числа від 1 до 5) через пробіл в одному рядку");
                    string[] inputs = Console.ReadLine().Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);

                    if (inputs.Length != size)
                    {
                        Console.WriteLine($"Помилка. Очікується {size} елементів, а введено {inputs.Length}.");
                        continue;
                    }

                    bool parseError = false;
                    for (int i = 0; i < size; i++)
                    {
                        if (!int.TryParse(inputs[i], out data[i]))
                        {
                            parseError = true;
                            break;
                        }
                    }

                    if (parseError)
                    {
                        Console.WriteLine("Помилка при читанні чисел. Перевірте правильність вводу.");
                        continue;
                    }
                }
                else
                {
                    Console.WriteLine("Невірний вибір. Почніть спочатку.");
                    continue;
                }

                Console.WriteLine("\nОтримано наступні вихідні дані");
                Console.WriteLine(string.Join(" ", data));

                // Виклик власного методу сортування вставками
                InsertionSort(data);

                Console.WriteLine("\nСформовано варіаційний ряд");
                Console.WriteLine(string.Join(" ", data));

                Console.WriteLine("\nРозраховано статистичний розподіл та характеристики");
                Console.WriteLine("Варіанта (xi) | Частота (mi) | Інтегральна частота | Частість (wi)");
                Console.WriteLine("------------------------------------------------------------------");

                int integralFrequency = 0;
                double sumWi = 0;
                List<string> wiValues = new List<string>();
                var groups = data.GroupBy(x => x).OrderBy(g => g.Key);

                foreach (var group in groups)
                {
                    int xi = group.Key;
                    int mi = group.Count();
                    integralFrequency += mi;

                    double wi = (double)mi / size;
                    sumWi += wi;
                    wiValues.Add(wi.ToString("F4"));

                    Console.WriteLine($"{xi,13} | {mi,12} | {integralFrequency,19} | {wi,13:F4}");
                }

                Console.WriteLine("\nКонтроль обчислень шляхом підсумовування частостей (wi):");
                Console.WriteLine(string.Join(" + ", wiValues) + $" = {sumWi:F4}");
            }
        }

        // Метод сортування вставками
        static void InsertionSort(int[] array)
        {
            for (int i = 1; i < array.Length; i++)
            {
                int key = array[i];
                int j = i - 1;

                // Переміщуємо елементи, які більші за key, на одну позицію вперед
                while (j >= 0 && array[j] > key)
                {
                    array[j + 1] = array[j];
                    j = j - 1;
                }
                array[j + 1] = key;
            }
        }
    }
}