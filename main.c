#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//Ορίζω τον αριθμό των μέγιστων νημάτων (MAX_THREADS), ώστε ο χρήστης να μην μπορεί να βάλει υπερβολκά μεγάλο αριθμό, καθώς έτσι μπορεί να προκύψουν διάφορα προβλήματα και επιπτώσεις στην απόδοση και στη λειτουργία του προγράμματος:
// 1. Αύξηση της χρήσης πόρων: Κάθε νήμα καταναλώνει πόρους συστήματος όπως μνήμη και χρόνο CPU. Αν αυξηθεί πολύ ο αριθμός των νημάτων, το σύστημα μπορεί να εξαντλήσει τους διαθέσιμους πόρους του, οδηγώντας σε καθυστερήσεις ή και σε αποτυχία εκτέλεσης του προγράμματος.
// 2. Αύξηση του overhead διαχείρισης νημάτων: Η δημιουργία και η διαχείριση των νημάτων έχει κάποιο overhead. Όταν ο αριθμός των νημάτων αυξάνεται σημαντικά, το λειτουργικό σύστημα ξοδεύει περισσότερο χρόνο στη διαχείριση αυτών των νημάτων, αντί στην πραγματική υπολογιστική εργασία.
// 3. Περιπτώσεις υπολειτουργίας του συστήματος (thrashing): Αν δημιουργηθούν πάρα πολλά νήματα, το σύστημα μπορεί να βρεθεί σε κατάσταση thrashing, όπου ξοδεύεται περισσότερος χρόνος στην εναλλαγή μεταξύ των νημάτων παρά στην εκτέλεση της υπολογιστικής εργασίας τους.
// 4. Ανταγωνισμός για τον μεταλλάκτη (mutex contention): Αν πολλά νήματα προσπαθούν να αποκτήσουν πρόσβαση στον ίδιο μεταλλάκτη ταυτόχρονα, θα υπάρξουν πολλές καθυστερήσεις καθώς τα νήματα περιμένουν τη σειρά τους να αποκτήσουν πρόσβαση. Αυτό μειώνει την αποδοτικότητα της παράλληλης επεξεργασίας.
// 5. Πιθανή μείωση της συνολικής απόδοσης: Σε ορισμένες περιπτώσεις, η χρήση υπερβολικά πολλών νημάτων μπορεί να οδηγήσει σε μείωση της συνολικής απόδοσης αντί σε αύξηση. Αυτό συμβαίνει επειδή το όφελος της παράλληλης επεξεργασίας υπερκαλύπτεται από τα παραπάνω προβλήματα.
#define MAX_THREADS 8


// Δομή δεδομένων για τα δεδομένα του κάθε νήματος
typedef struct {
    int *array;       // Δείκτης στον πίνακα των δεδομένων
    int start;        // Αρχική θέση για το νήμα
    int end;          // Τελική θέση για το νήμα
    long long local_sum; // Τοπικό άθροισμα για το νήμα
} ThreadData;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Αρχικοποίηση μεταλλάκτη
long long total_sum = 0; // Συνολικό άθροισμα

// Αρχικοποίηση συναρτήσεων
void* thread_sum(void* );

int main() {
    int n, t, i; // Αρχικοποίηση μεταβλητών

    // Εισαγωγή του αριθμού των στοιχείων του πίνακα
    printf("Enter the number of elements (n): ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        fprintf(stderr, "Invalid input for number of elements\n");
        return 1;
    }

    // Εισαγωγή του αριθμού των νημάτων
    printf("Enter the number of threads (t): ");
    if (scanf("%d", &t) != 1 || t <= 0 || t > MAX_THREADS) {
        fprintf(stderr, "Invalid input for number of threads\n");
        return 1;
    }

    // Έλεγχος αν το n είναι ακέραιο πολλαπλάσιο του t
    if (n % t != 0) {
        fprintf(stderr, "n must be an integer multiple of t\n");
        return 1;
    }

    // Δέσμευση μνήμης για τον πίνακα
    int *array = (int*) malloc(n * sizeof(int));
    if (array == NULL) {
        perror("Failed to allocate memory");
        return 1;
    }

    // Γέμισμα του πίνακα με τιμές, σύμφωνα με την υπόδειξη). Το θέτω ίσο με το i ώστε να μην είναι εντελώς τυχαίες οι τιμές και να μπορώ να συκγρίνω την ίδια διαδικασία με διαφορετικό αριθμό νημάτων
    for (i = 0; i < n; ++i) {
        array[i] = i;
    }

    pthread_t threads[MAX_THREADS]; // Πίνακας για τα νήματα
    ThreadData thread_data[MAX_THREADS]; // Πίνακας για τα δεδομένα των νημάτων
    int chunk_size = n / t; // Μέγεθος του τμήματος που θα επεξεργάζεται κάθε νήμα

    clock_t start_time = clock(); // Έναρξη χρονομέτρησης

    // Δημιουργία και εκκίνηση των νημάτων
    for (i = 0; i < t; ++i) {
        thread_data[i].array = array;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i + 1) * chunk_size;
        thread_data[i].local_sum = 0;

        if (pthread_create(&threads[i], NULL, thread_sum, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            free(array);
            pthread_mutex_destroy(&mutex);
            return 1;
        }
    }

    // Αναμονή για την ολοκλήρωση όλων των νημάτων
    for (i = 0; i < t; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Failed to join thread");
            free(array);
            pthread_mutex_destroy(&mutex);
            return 1;
        }
    }

    clock_t end_time = clock(); // Λήξη χρονομέτρησης

    // Υπολογισμός και εκτύπωση του μέσου όρου των τετραγώνων
    double average = (double)total_sum / n;
    printf("Average of squares: %.2f\n", average);
    printf("Execution time: %.2f seconds\n", (double)(end_time - start_time));

    free(array); // Αποδέσμευση της μνήμης
    pthread_mutex_destroy(&mutex); // Καταστροφή του μεταλλάκτη

    return 0;
}

// Συνάρτηση που θα εκτελεί κάθε νήμα
void* thread_sum(void* arg) {
    ThreadData *data = (ThreadData*) arg;
    data->local_sum = 0;
    int i;

    // Υπολογισμός του τοπικού αθροίσματος των τετραγώνων των στοιχείων του πίνακα
    for (i = data->start; i < data->end; ++i) {
        data->local_sum += (long long)data->array[i] * data->array[i];
    }

    // Κλείδωμα του μεταλλάκτη για να ενημερώσει το συνολικό άθροισμα
    pthread_mutex_lock(&mutex);
    total_sum += data->local_sum;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

