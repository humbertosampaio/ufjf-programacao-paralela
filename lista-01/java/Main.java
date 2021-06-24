public class Main {

    static long calcularFibonacci(int n) {
        return (n < 2) ? n : calcularFibonacci(n - 1) + calcularFibonacci(n - 2);
    }

    public static void main() {
        for (int i = 0; i < 50; i++) {
            System.out.println("(" + i + "):" + calcularFibonacci(i));
        }
    }
}